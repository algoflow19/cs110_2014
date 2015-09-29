/**
 * File: http-proxy-scheduler.h
 * ----------------------------
 * This class defines the scheduler class, which takes all
 * proxied requests off of the main thread and schedules them to 
 * be handled by a constant number of child threads.
 *
 * Note: The starter version of the code doesn't enlist the services
 * of the ThreadPool, and instead just handles the request sequentially
 * on the main thread.  That will ultimately need to change, though.
 */

#ifndef _http_proxy_scheduler_
#define _http_proxy_scheduler_

#include <string>
#include <utility>
#include "request-handler.h"
#include "thread-pool.h"

class HTTPProxyScheduler {
 public:
/**
 * Constructor
 * Used to initialize threadpool
 */
  HTTPProxyScheduler();

/**
 * Function: scheduleRequest
 * --------------------------
 *  Schedules http requests to the threadpool
 */
  void scheduleRequest(int clientfd, const std::string& clientIPAddr, std::string proxyServer, unsigned short proxyPortNumber, bool usingProxy);
  
 private:
  #define ALLOWED_THREADS 16
  /*
   * Thread pool
   */
  ThreadPool handleRequestPool;
  HTTPRequestHandler requestHandler;
};

#endif
