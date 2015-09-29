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

class HTTPRequestHandler {
 public:
  void serviceRequest(const std::pair<int, std::string>& connection) throw();
};

#endif
