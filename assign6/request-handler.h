/**
 * File: request-handler.h
 * -----------------------
 * Defines the HTTPRequestHandler class, which fully proxies and
 * services a single client request.  
 */

#ifndef _http_request_handler_
#define _http_request_handler_

#include <utility>
#include <string>
#include <socket++/sockstream.h> // for sockbuf, iosockstream
#include "client-socket.h"
#include "request.h"
#include "response.h"
#include "blacklist.h"
#include "cache.h"
#include "ostreamlock.h"

class HTTPRequestHandler {
  private:
  #define DEFAULTHTTPPORT 80
  HTTPCache cache;
  HTTPBlacklist blacklist;
  void fillResponseForProxyChainException(HTTPResponse& response, const std::string& message);
  void fillResponseForBlackList(HTTPResponse& response, const std::string& message);
  void fillBadRequestResponse(HTTPResponse& response, const std::string& message);
  void requestLog(const HTTPRequest& request);
  void log(const std::string& message);
  bool ingestRequestLine(HTTPRequest& request, iosockstream& ss, HTTPResponse& response);
  bool setForwardedFor(HTTPRequest& request, iosockstream& ss, HTTPResponse& response, const std::pair<int, std::string>& connection);
  public :
  HTTPRequestHandler();
  void serviceRequest(const std::pair<int, std::string>& connection, std::string proxyServer, unsigned short proxyPortNumber, bool usingProxy) throw();
};

#endif
