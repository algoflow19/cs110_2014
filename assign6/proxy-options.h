/**
 * File: proxy-options.h
 * ---------------------
 * Defines a collection of functions that can be
 * uses to parse the command line options available
 * to http-proxy.  In particular, the following
 * option flags are available to the http-proxy:
 *
 *  --port <port-number>: allows the user to specify a specific port to 
 *                        use instead of the user-specific default
 *  --proxy <proxy-server>: allows the proxy to itself direct traffic to
 *                          another proxy. We impose the simplification that
 *                          the ports for the primary and secondary proxies
 *                          be there same (they would need to be the same in
 *                          practice, but it simplifies parsing considerably.)
 */

#pragma once

#include "proxy-exception.h"
#include <string>

/**
 * Function: computeDefaultPortForUser
 * -----------------------------------
 * Constructs the default port for the logged in user.  The
 * port uses the logged in user's SUNet ID to generate a
 * port number that, with high probability, it not likely to
 * be used by any other users.
 */
unsigned short computeDefaultPortForUser();
unsigned short extractPortNumber(const char *portArgument, const char *flags) throw (HTTPProxyException);
std::string extractProxyServer(const char *proxyArgument) throw (HTTPProxyException);
