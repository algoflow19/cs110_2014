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

static const size_t kFeedsPoolSize = 6;
static const size_t kArticlesPoolSize = 12;

static bool verbose = false;
static ThreadPool feedsPool(kFeedsPoolSize);
static ThreadPool articlesPool(kArticlesPoolSize);
static RSSIndex index;
static mutex indexLock;

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
 * Accepts the URL of an RSS News Feed List, downloads it, and for each
 * RSS News Feed URL within it, schedules that URL to be fully processed by creating
 * a closure that does precisely that and adding it to feedsPool.
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
  
  // add code to schedule each of the RSS feeds to be downloaded
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
