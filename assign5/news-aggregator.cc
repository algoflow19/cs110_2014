/**
 * File: news-aggregator.cc
 * ------------------------
 * Provides the solution for the CS110 News Aggregator 
 * assignment.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include <libxml/parser.h>
#include <libxml/catalog.h>
#include <mutex>
#include <getopt.h>
#include <unordered_set>
#include "article.h"
#include "thread-pool.h"
#include "rss-feed-list.h"
#include "rss-feed.h"
#include "rss-index.h"
#include "html-document.h"
#include "html-document-exception.h"
#include "rss-feed-exception.h"
#include "rss-feed-list-exception.h"
#include "news-aggregator-utils.h"
#include "ostreamlock.h"
#include "string-utils.h"
using namespace std;

/*
 * Globals to specify the size of thread pool
 */
static const size_t kFeedsPoolSize = 6;
static const size_t kArticlesPoolSize = 12;

static bool verbose = false;

/*
 * Thread pools to process RSS feeds and articles
 */
static ThreadPool feedsPool(kFeedsPoolSize);
static ThreadPool articlesPool(kArticlesPoolSize);
static RSSIndex index;
// Mutex to guard global index
static mutex indexLock;

/*
 * Thread safe functions
 */
static void addToIndex(const Article& article, const vector<string>& tokens);

/*
 * Threads
 */
static void processArticle(Article article, const string url);
static void processRSSFeed(const string url, const string rss);

/**
 * Informs the user exactly how the executable can be invoked.
 */
static const int kIncorrectUsage = 1;
static void printUsage(const string& executableName, const string& message = "") {
  if (!message.empty()) cerr << "Error: " << message << endl;
  cerr << "Usage: " << executableName << " [--verbose] [--quiet] [--url <feed-file>]" << endl;
  exit(kIncorrectUsage);
}

/**
 * Parses the argument list to search for optional --verbose, --quiet, and
 * --url flags.  If anything bogus is provided (unrecognized flag, --url is missing
 * its argument, etc., then a usage message is printed and the program is terminated.
 */
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
    case '?':
      printUsage(argv[0]);
    default:
      printUsage(argv[0], "Unrecognized flag.");
    }
  }
  
  argc -= optind;
  argv += optind;
  if (argc > 0) {
    printUsage(argv[0], "Too many arguments.");
  }

  return url;
}

/**
  * Function: addToIndex
  * ---------------------
  *  Adds article and its tokens to the Global indexer.
  *  Discards duplicate articles.
  *  Note : Thread safe
  */
static void addToIndex(const Article& article, const vector<string>& tokens) {
    // Using lock_guard to avoid explicitly calling lock/unlock
    lock_guard<mutex> lg(indexLock);
    index.add(article, tokens);
}

/**
 * Function: processArticle
 * ----------------------------
 *  The following function is the definition of the processArticle grandchild thread.
 *  It will be scheduled by processRSSFeed child thread.
 *  Note : Thread safe
 */
static void processArticle(const Article article, const string url) {
    HTMLDocument document(article.url);
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
        cerr << oslock << "Ran into trouble while pulling HTML document from \""
             <<  article.url << endl << "Ignoring..." << endl << osunlock;
        return;
    }
    const vector<string>& tokens = document.getTokens();
    // Adds to global index
    addToIndex(article, tokens);
}

/**
 * Function: processRSSFeed
 * ----------------------------
 *  The following function is the definition of the processRSSFeed child thread.
 *  It will be schduled by the main thread via processAllFeeds()
 *  Note : Thread safe.
 */
static void processRSSFeed(const string url, const string rss) {

    if (verbose) {
        cout << oslock << "Begin full download of feed URI: "
             << (shouldTruncate(url) ? truncate(url):url) << endl << osunlock;
    }
    RSSFeed feed(url);
    try {
        feed.parse();
    } catch (const RSSFeedException& rfe) {
        cerr << oslock << "Ran into trouble while pulling RSS feed from \""
             << /*(shouldTruncate(url) ? truncate(url):url)*/ url << "\"." << endl
             << "Ignoring..." << endl << osunlock;
        return;
    }

    // Acquiring the list of articles specific to this RSS feed
    const vector<Article>& articles = feed.getArticles();

    // Creating a copy of every article in articles vector
    // Avoided passing arguments by reference 
    for (const Article article : articles) {
        // Scheduling the articles to be downloaded
        articlesPool.schedule(
                              // Forcing copy of article and url by specifying them in
                              // lambda 
                              [article, url] () {
                                    processArticle(article, url);
                              }
                             );
    }

    if (verbose) {
        cout << oslock << rss  
             << ": All articles have been scheduled." << endl
             << osunlock;
    }
}

/**
 * Function: processAllFeeds
 * ----------------------------
 *  Parses rss feed lists, downloads and indexes articles contained in them.
 *  Note : Spawns threads; theoretical maximum of kFeedsPoolSize + 
 *  kArtilcesPoolSize + 2 (dispatcher) active threads.
 */
static const int kBogusRSSFeedListName = 1;
static void processAllFeeds(const string& feedListURI) {
  RSSFeedList feedList(feedListURI);
  try {
    feedList.parse();
  } catch (const RSSFeedListException& rfle) {
    cerr << "Ran into trouble while pulling full RSS feed list from \""
	 << feedListURI << "\"." << endl; 
    cerr << "Aborting...." << endl;
    exit(kBogusRSSFeedListName);
  }

  // Map of Rss feed title and its corresponding server url 
  const map<string, string>& feeds = feedList.getFeeds();

  for (pair<string, string> entry : feeds) {
    // Scheduling the RSS feeds to be processed
    feedsPool.schedule(
                       // Forcing copy of entry by specifying it in lambda
                       [entry] () {
                            processRSSFeed(entry.first, entry.second);
                       }
                      );
    
  }
  feedsPool.wait();
  if (verbose)
  cout << "All RSS news feed documents have been downloaded!" << endl;
  articlesPool.wait();
  if (verbose)
  cout << "All news articles have been downloaded!" << endl;
}

/**
 * Assumes that the global RSSIndex has been fully populated with the full index of
 * all words in all relevent news articles, so that the user can perform a standard
 * lookup of single words.  This function, not surprisingly, should be executed in
 * a sequential part of the program, where no other threads are running any more.
 */
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

/**
 * Defines the entry point for the entire program.  Save for
 * Some xmllib configuration, the main function amounts to little more
 * that an outline of two function calls to processAllFeeds (which
 * uses a single ThreadPool to download RSS feeds and a second ThreadPool
 * to download and index all of the RSS feeds' news articles) and queryIndex.
 */
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
