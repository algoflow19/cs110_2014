/**
 * File: request-handler.cc
 * ------------------------
 * Provides the implementation for the HTTPRequestHandler class.
 */

#include "request-handler.h"

using namespace std;

/**
 * Constructor: HTTPRequestHandler
 * ---------------------------------
 *  Used to intialize private data member blacklist
 */
HTTPRequestHandler::HTTPRequestHandler() : blacklist("blocked-domains.txt") {}

/**
 * Method: fillBadRequestResponse
 * --------------------------------
 *  Fills us response code, protocol and payload for bad http requests.
 */
inline void HTTPRequestHandler::fillBadRequestResponse(HTTPResponse &response, const std::string& message) {
    response.setResponseCode(400);
    response.setProtocol("HTTP/1.0");
    response.setPayload(message);
}

/**
 * Method: fillResponseForProxyChainException 
 * ---------------------------------------------
 *  Fills us response code, protocol and payload for Proxy chain exceptions.
 */
inline void HTTPRequestHandler::fillResponseForProxyChainException(HTTPResponse& response, const std::string& message) {
    response.setResponseCode(504);
    response.setProtocol("HTTP/1.0");
    response.setPayload(message);
}

/**
 * Method: fillResponseForBlackList 
 * ---------------------------------
 *  Fills us response code, protocol and payload for blaclisted http requests.
 */
inline void HTTPRequestHandler::fillResponseForBlackList(HTTPResponse& response, const std::string& message) {
    response.setResponseCode(403);
    response.setProtocol("HTTP/1.1");
    response.setPayload(message);
}

/**
 * Method: requestLog 
 * --------------------------------
 *  Prints request log. Thread safe. 
 */
inline void HTTPRequestHandler::requestLog(const HTTPRequest& request) {
    cout << oslock << request.getMethod() << " " << request.getURL() << endl << osunlock;
}

/**
 * Method: log 
 * --------------------------------
 *  Prints log message. Thread safe.
 */
inline void HTTPRequestHandler::log(const std::string& message) {
    cout << oslock << message << endl << osunlock;
}

/**
 * Method: ingestRequestLine
 * ---------------------------
 *  Ingests request line from incoming http request. Handles exceptions.
 *  Returns false in case of exception. Returns true on success.
 */
bool HTTPRequestHandler::ingestRequestLine(HTTPRequest& request, iosockstream& ss, HTTPResponse& response) {
  try {
    request.ingestRequestLine(ss);
  } catch (const HTTPBadRequestException& hbre) {
    /* Catching Bad request exception */
    /* Log */
    log(hbre.what());
    /* Build response for bad request */
    fillBadRequestResponse(response, hbre.what());
    ss << response;
    ss.flush();
    return false;
  }
  return true;
}

/**
 * Method: ingestRequestLine
 * ---------------------------
 *  Sets forwarded for field in incoming http request. Handles exceptions.
 *  Returns false in case of exception. Returns true on success.
 */
bool HTTPRequestHandler::setForwardedFor(HTTPRequest& request, iosockstream& ss, HTTPResponse& response, const std::pair<int, string>& connection) {
  try {
    request.setForwardedFor(connection.second);
  } catch (const HTTPCircularProxyChainException& hcpce) {
    /* Error code for Circular Proxy */
    fillResponseForProxyChainException(response, hcpce.what());
    ss << response;
    ss.flush();
    return false;
  }
  return true;
}

/**
 * Method: serviceRequest 
 * --------------------------------
 * The following method is used to service incoming HTTP requests from clients/proxies.
 * Thread safe.
 */
void HTTPRequestHandler::serviceRequest(const std::pair<int, std::string>& connection, std::string proxyServer, unsigned short proxyPortNumber, bool usingProxy) throw() {
  /* Creating request, response objects */
  HTTPRequest request(usingProxy);
  HTTPResponse response;
  sockbuf sb(connection.first);
  /* Socstream facing incoming client */
  iosockstream ss(&sb);

  /* Ingesting request */
  if (ingestRequestLine(request, ss, response) == false)
      return;

  request.ingestHeader(ss, connection.second);
  request.ingestPayload(ss);
  /* Logging request */
  requestLog(request);

  /* Setting appropriate http fields */
  request.setForwardedProto();
  if (setForwardedFor(request, ss, response, connection) == false)
      return;

  /* Check if request is part of blacklist */
  if (blacklist.serverIsAllowed(request.getServer()) == true) {
    /* Lock potential cache entry */
    cache.cacheLock(request);
    int clientSocket;
    /* Open socket facing proxy server or origin server */
    if (cache.containsCacheEntry(request, response) == false) {
      if (usingProxy)
          clientSocket = createClientSocket(proxyServer, proxyPortNumber);
      else
          clientSocket = createClientSocket(request.getServer(), DEFAULTHTTPPORT);
      sockbuf fsb(clientSocket);
      /* Sockstream facing origin server or proxy */
      iosockstream fss(&fsb);
      /* Sending request */
      fss << request;
      fss.flush();

      /*Ingesting response */
      response.ingestResponseHeader(fss);
      response.ingestPayload(fss);
      /*Check if it needs to be cached */
      if (cache.shouldCache(request, response) == true) {
          cache.cacheEntry(request, response);
      }
    } 
    /* Unlocking cache */
    cache.cacheUnlock(request);
  } else {
    /* Filling up response for blacklisted http request */
    fillResponseForBlackList(response, "Forbidden Content");
  }
  /* Sending response back to client */
  ss << response;
  ss.flush();
}
