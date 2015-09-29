/**
 * File: scheduler.cc
 * ------------------
 * Presents the implementation of the HTTPProxyScheduler class.
 */

#include "scheduler.h"

using namespace std;

/**
 * Constructor: HTTPProxyScheduler
 * -------------------------------
 *  Constructor for HTTPProxyScheduler class
 *  It is used to initialize handlerequest threadpool
 */
HTTPProxyScheduler::HTTPProxyScheduler() : handleRequestPool(ALLOWED_THREADS) {}

/**
 * Method: scheduleRequest
 * -----------------------------
 * Schedules http request to the handle request threadpool
 */
void HTTPProxyScheduler::scheduleRequest(int clientfd, const string& clientIPAddress, std::string proxyServer, unsigned short proxyPortNumber, bool usingProxy) {
  const pair<int, string> connection = make_pair(clientfd, clientIPAddress);
  handleRequestPool.schedule(
                             // Forces hardcopy of all data that will be used inside the 
                             // lambda closure. 
                             [this, connection, proxyServer, proxyPortNumber, usingProxy] () {
                                 requestHandler.serviceRequest(connection, proxyServer, proxyPortNumber, usingProxy);
                             }
                            );
}
