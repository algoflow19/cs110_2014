/**
 * File: cache.h
 * -------------
 * Defines a class to help manage an 
 * HTTP response cache.
 */

#ifndef _http_cache_
#define _http_cache_

#include <cstdlib>
#include <string>
#include <mutex>
#include "request.h"
#include "response.h"

class HTTPCache {
 public:

/**
 * Constructs the HTTPCache object.
 */
  HTTPCache();

/**
 * The following three functions do what you'd expect, except that they 
 * aren't thread safe.  In a MT environment, you should acquire the lock
 * on the relevant request before calling these.
 *
 * Note : Need to acquire cacheLock before calling any of these
 */
  bool containsCacheEntry(const HTTPRequest& request, HTTPResponse& response) const;
  bool shouldCache(const HTTPRequest& request, const HTTPResponse& response) const;
  void cacheEntry(const HTTPRequest& request, const HTTPResponse& response);
/**
 * The following public mutex methods have to be used to prevent
 * race conditions in fetching and creating cache entries
 */
  void cacheLock(const HTTPRequest& request);
  void cacheUnlock(const HTTPRequest& request);
  
 private:
  #define MAX_MUTEXES 1001
  size_t hashRequest(const HTTPRequest& request) const;
  std::string hashRequestAsString(const HTTPRequest& request) const;
  std::string serializeRequest(const HTTPRequest& request) const;
  bool cacheEntryExists(const std::string& filename) const;
  std::string getRequestHashCacheEntryName(const std::string& requestHash) const;
  void ensureDirectoryExists(const std::string& directory, bool empty = false) const;
  std::string getExpirationTime(int ttl) const;
  bool cachedEntryIsValid(const std::string& cachedFileName) const;
  std::string cacheDirectory;
  std::mutex cacheLocks[MAX_MUTEXES];
};

#endif
