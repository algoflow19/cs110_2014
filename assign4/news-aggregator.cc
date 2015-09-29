/**
 * File: news-aggregator.cc
 * ------------------------
 * When fully implemented, news-aggregator.cc pulls, parses,
 * and indexes every single news article reachable from some
 * RSS feed in the user-supplied RSS News Feed XML file, 
 * and then allows the user to query the index.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/catalog.h>
#include <getopt.h>
#include <thread>
#include <map>
#include <unordered_set>
#include "semaphore.h"
#include "ostreamlock.h"
#include "thread-utils.h"
#include "article.h"
#include "rss-feed-list.h"
#include "rss-feed.h"
#include "rss-index.h"
#include "html-document.h"
#include "html-document-exception.h"
#include "rss-feed-exception.h"
#include "rss-feed-list-exception.h"
#include "news-aggregator-utils.h"
#include "string-utils.h"

// To avoid pesky std:: suffixes
using namespace std;

// Macros
#define MAX_CONCURRENTLY_ALLOWED_RSS_SERVERS 6
#define MAX_CONCURRENTLY_ALLOWED_ARTICLES_PER_SERVER 10
#define MAX_CONCURRENTLY_ALLOWED_ARTICLES 32

typedef struct bcd{
        bool available;
            unique_ptr<semaphore> workerSemaphore;
} workerStatus;

// globals
static bool verbose = false;
static RSSIndex index;

// Global semaphore to govern concurrent processing of RSS servers.
static semaphore RSSServersSem(MAX_CONCURRENTLY_ALLOWED_RSS_SERVERS);
// Global semaphore to govern concurrent processing of Articles. 
static semaphore maxArticlesSem(MAX_CONCURRENTLY_ALLOWED_ARTICLES);
// Mutex to guard global index
static mutex indexLock;
// Mutex to guard serverSemaphore map and indexedUrl set
static mutex mapLock;
// Map of server urls and their associated semaphores.
static map<string, unique_ptr<semaphore>> serverSemaphores;
// Unordered set (amortized O(1) for search) to keep track of indexed articles
static unordered_set<string> indexedUrl;
static const int kBogusRSSFeedListName = 1;
static const int kIncorrectUsage = 1;

/*
 * Helper functions
 */
static inline bool alreadyIndexed(const string& url);
static inline void addAsIndexed(const string& url);

/*
 * Thread safe functions
 */
static void addToIndex(Article& article, const vector<string>& tokens);
static void serverSemaphoreWait(const string& url);
static void serverSemaphoreSignal(const string& url);

/*
 * Threads
 */
static void processArticle(Article& article, const string& url);
static void processRSSFeed(const string& url);


static void printUsage(const string& message, const string& executableName) {
  cerr << "Error: " << message << endl;
  cerr << "Usage: " << executableName << " [--verbose] [--quiet] [--url <feed-file>]" << endl;
  exit(kIncorrectUsage);
}

static const string kDefaultRSSFeedListURL = "small-feed.xml";
static string parseArgumentList(int argc, char *argv[]) {
  struct option options[] = {
    {"verbose", no_argument, NULL, 'v'},
    {"quiet", no_argument, NULL, 'q'},
    {"url", required_argument, NULL, 'u'},
    {NULL, 0, NULL, 0},
  };

  string url = kDefaultRSSFeedListURL;
  while (true) {
    int ch = getopt_long(argc, argv, "vqu:", options, NULL);
    if (ch == -1) break;
    switch (ch) {
    case 'v':
      verbose = true;
      break;
    case 'q':
      verbose = false;
      break;
    case 'u':
      url = optarg;
      break;
    default:
      printUsage("Unrecognized flag.", argv[0]);
    }
  }
  
  argc -= optind;
  argv += optind;
  if (argc > 0) {
    printUsage("Too many arguments", argv[0]);
  }
  return url;
}

/**
 * Function: alreadyIndexed
 * -------------------------
 *  Returns true if the server had already been indexed
 *  Note : Not thread safe, mutexes have to be acquired
 *  prior to call.
 */
static inline bool alreadyIndexed(const string& url) {
    return (indexedUrl.find(url) != indexedUrl.end());
}

/**
 * Function: addAsIndexed
 * ------------------------
 *  Inserts server to the set of servers 
 *  Note : Not thread safe, mutexes have to be acquired
 *  prior to call.
 */
static inline void addAsIndexed(const string& url) {
    indexedUrl.insert(url);
}

/**
 * Function: addToIndex
 * ---------------------
 *  Adds article and its tokens to the Global indexer.
 *  Discards duplicate articles.
 *  Note : Thread safe
 */
static void addToIndex(Article& article, const vector<string>& tokens) {
    // Using lock_guard to avoid explicitly calling lock/unlock
    lock_guard<mutex> lg(indexLock);
    if (!alreadyIndexed(article.url)) {
        index.add(article, tokens);
        addAsIndexed(article.url);
    }
}

/**
 * Function: serverSemaphoreWait 
 * -----------------------------------
 *  Acquires relevant Server semaphore associated with url.
 *  Creates new semaphores for Servers not present in serverSemaphores map
 *  Note : Thread safe
 */
static void serverSemaphoreWait(const string& url) {
    // Acquiring mutex governing global map and set data structures.
    mapLock.lock();
    unique_ptr<semaphore>& up = serverSemaphores[getURLServer(url)];
    if (up == nullptr) {
        up.reset(new semaphore(MAX_CONCURRENTLY_ALLOWED_ARTICLES_PER_SERVER));
    }
    mapLock.unlock();
    up->wait();
}

/**
 * Function: serverSemaphoreSignal 
 * -----------------------------------
 *  Signals relevant Server semaphore associated with url on thread exit.
 *  Note : Thread safe
 */
static void serverSemaphoreSignal(const string& url) {
    // Acquiring mutex governing global map and set data structures.
    mapLock.lock();
    unique_ptr<semaphore>& up = serverSemaphores[getURLServer(url)];
    if (up == nullptr) {
        // Highly unlikely
        mapLock.unlock();
        cerr << oslock 
             << "Semaphore does not exist to signal waiting threads for Server: " 
             << endl << "\t[at \"" << (shouldTruncate(getURLServer(url)) ? 
                                       truncate(getURLServer(url)) : 
                                       getURLServer(url)) 
             << "\"]" << endl << osunlock;
        return;
    }
    mapLock.unlock();
    up->signal(on_thread_exit);
}

/**
 * Function: processArticle
 * ----------------------------
 *  The following function is the definition of the processArticle grandchild thread.
 *  It will be spawned by processRSSFeed child thread.
 *  Note : Thread safe
 */
static void processArticle(Article& article, const string& url) {
    HTMLDocument document(article.url);
    // Wait if we exceed max active connection count
    maxArticlesSem.wait();
    if (verbose) {
        cout << oslock << "  Parsing \"" 
             << (shouldTruncate(article.title) ? truncate(article.title) : article.title) 
             << "\"" << endl << "\t[at \""
             << (shouldTruncate(article.url) ? truncate(article.url) : article.url) 
             << "\"]" << endl << osunlock;
    }

    try {
        document.parse();
    } catch (const HTMLDocumentException hde) {
        cerr << oslock << "Ran into trouble while pulling article \""
             << (shouldTruncate(article.title) ? truncate(article.title) : article.title)
             << "\"." << endl << "Ignoring..." << endl << osunlock;
        serverSemaphoreSignal(url);
        maxArticlesSem.signal(on_thread_exit);
        return;
    }
    const vector<string> tokens = document.getTokens();
    // Adds to global index
    addToIndex(article, tokens);
    serverSemaphoreSignal(url);
    // Signal to waiting threads on exit.
    maxArticlesSem.signal(on_thread_exit);
}

/**
 * Function: processRSSFeed
 * ----------------------------
 *  The following function is the definition of the processRSSFeed child thread.
 *  It will be spawned by the main thread via processAllFeeds() 
 *  Note : Thread safe, assumes RSSServersSem to be acquired prior to call.
 */
static void processRSSFeed(const string& url) {
    if (verbose) {
        cout << oslock << "Begin full download of feed URI: " 
             << truncate(url) << endl << osunlock;
    }

    RSSFeed feed(url);
    try {
       feed.parse();
    } catch (const RSSFeedException& rfe) {
        cerr << oslock << "Ran into trouble while pulling RSS feed from \""
             << (shouldTruncate(url) ? truncate(url):url) << "\"." << endl 
             << "Ignoring..." << endl << osunlock;
        RSSServersSem.signal(on_thread_exit);
        return;
    }

    // Acquiring the list of articles specific to this RSS feed
    const vector<Article>& articles = feed.getArticles();

    // Spawning grand child threads to download individual articles.
    vector<thread> articleThreads;
    for (const Article& article : articles) {
        serverSemaphoreWait(url);
        articleThreads.push_back(thread(processArticle, article, url));
    }

    for (thread& t: articleThreads) t.join();

    if (verbose) {
        cout << oslock << "End full download of feed URI: " << truncate(url) << endl 
             << osunlock;
    }
    
    // Signal to waiting threads on exit.
    RSSServersSem.signal(on_thread_exit);
}

/**
 * Function: processAllFeeds
 * ----------------------------
 *  Parses rss feed lists, downloads and indexes articles contained in them.
 *  Note : Spawns threads; theoretical maximum of 60 active threads
 */
static void processAllFeeds(const string& feedListURI) {
    RSSFeedList feedList(feedListURI);
    try {
        feedList.parse();
    } catch (const RSSFeedListException& rfle) {
        cerr << "Ran into trouble while pulling full RSS feed list from \""
	         << feedListURI << "\"." << endl << "Aborting...." << endl; 
        exit(kBogusRSSFeedListName);
    }

    const map<string, string>& feeds = feedList.getFeeds(); 

    // Spawning individual child threads for every rssfeed identified
    vector<thread> RSSThreads;
    for (map<string, string>::const_iterator itr = feeds.begin(), itr_end = feeds.end(); itr != itr_end; ++itr) {
        RSSServersSem.wait();
        RSSThreads.push_back(thread(processRSSFeed, itr->first));
    }

    // Accounting for all child threads spawned.
    for (thread& t: RSSThreads) t.join();
}

static const size_t kMaxMatchesToShow = 15;
static void queryIndex() {
  while (true) {
    cout << "Enter a search term [or just hit <enter> to quit]: ";
    string response;
    getline(cin, response);
    response = trim(response);
    if (response.empty()) break;
    const vector<pair<Article, int> >& matches = index.getMatchingArticles(response);
    if (matches.empty()) {
      cout << "Ah, we didn't find the term \"" << response << "\". Try again." << endl;
    } else {
      cout << "That term appears in " << matches.size() << " article" 
	   << (matches.size() == 1 ? "" : "s") << ".  ";
      if (matches.size() > kMaxMatchesToShow) 
	cout << "Here are the top " << kMaxMatchesToShow << " of them:" << endl;
      else if (matches.size() > 1)
	cout << "Here they are:" << endl;
      else
        cout << "Here it is:" << endl;
      size_t count = 0;
      for (const pair<Article, int>& match: matches) {
	if (count == kMaxMatchesToShow) break;
	count++;
	string title = match.first.title;
	if (shouldTruncate(title)) title = truncate(title);
	string url = match.first.url;
	if (shouldTruncate(url)) url = truncate(url);
	string times = match.second == 1 ? "time" : "times";
	cout << "  " << setw(2) << setfill(' ') << count << ".) "
	     << "\"" << title << "\" [appears " << match.second << " " << times << "]." << endl;
	cout << "       \"" << url << "\"" << endl;
      }
    }
  }
}

int main(int argc, char *argv[]) {
  string rssFeedListURI = parseArgumentList(argc, argv);
  xmlInitParser();
  xmlInitializeCatalog();
  processAllFeeds(rssFeedListURI);
  xmlCatalogCleanup();
  xmlCleanupParser();
  queryIndex();
  return 0;
}
