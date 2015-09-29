/**
 * File: http-request.cc
 * ---------------------
 * Presents the implementation of the HTTPRequest class and
 * its friend functions as exported by request.h.
 */

#include <sstream>
#include "request.h"
#include "string-utils.h"
using namespace std;

static const string kWhiteSpaceDelimiters = " \r\n\t";
static const string kProtocolPrefix = "http://";
static const unsigned short kDefaultPort = 80;
HTTPRequest::HTTPRequest() : usingProxy(false) {}
HTTPRequest::HTTPRequest(bool usingProxy) : usingProxy(usingProxy) {}
void HTTPRequest::ingestRequestLine(istream& instream) throw (HTTPBadRequestException) {
  getline(instream, requestLine);
  if (instream.fail()) {
    throw HTTPBadRequestException("First line of request could not be read.");
  }

  requestLine = trim(requestLine);
  istringstream iss(requestLine);
  iss >> method >> url >> protocol;
  server = url;
  size_t pos = server.find(kProtocolPrefix);
  server.erase(0, kProtocolPrefix.size());
  pos = server.find('/');
  if (pos == string::npos) {
    // url came in as something like http://www.google.com, without the trailing /
    // in that case, least server as is (it'd be www.google.com), and manually set
    // path to be "/"
    path = "/";
  } else {
    path = server.substr(pos);
    server.erase(pos);
  }
  port = kDefaultPort;
  pos = server.find(':');
  if (pos == string::npos) return;
  port = strtol(server.c_str() + pos + 1, NULL, 0); // assume port is well-formed
  server.erase(pos);
}

void HTTPRequest::ingestHeader(istream& instream, const string& clientIPAddress) {
  requestHeader.ingestHeader(instream);
}

bool HTTPRequest::containsName(const string& name) const {
  return requestHeader.containsName(name);
}

void HTTPRequest::ingestPayload(istream& instream) {
  if (getMethod() != "POST") return;
  payload.ingestPayload(requestHeader, instream);
}

ostream& operator<<(ostream& os, const HTTPRequest& rh) {
  if (rh.usingProxy == true) {
  // If using proxy, send method, url and protocol in request line
    os << rh.method << " " << rh.url << " " << rh.protocol << "\r\n";
  } else {
  // If not using proxy, send method, path and protocol in request line
  const string& path = rh.path;
  os << rh.method << " " << path << " " << rh.protocol << "\r\n";
  }
  os << rh.requestHeader;
  os << "\r\n"; // blank line not printed by request header
  os << rh.payload;
  return os;
}

/**
 * Method: setForwardedProto
 * --------------------------
 *  Adds or overwrites the x-forwarded-proto field with value "http" in http header
 */
void HTTPRequest::setForwardedProto() {
    requestHeader.addHeader("x-forwarded-proto", "http");  
}

/**
 * Method: setForwardedFor
 * -------------------------
 * Adds or updates the x-forwarded-for field with ip address of the client.
 * Throws HTTPCircularProxyChainException exception when detected
 */
void HTTPRequest::setForwardedFor(const std::string& clientIPAddress) throw (HTTPCircularProxyChainException) {
    if (containsName("x-forwarded-for") == true) {
        // When field already exists
        /* If client ip address already present, throw an exception */
        if (requestHeader.getValueAsString("x-forwarded-for").find(clientIPAddress) != string::npos) {
            throw HTTPCircularProxyChainException("Chained proxies may not lead through a cycle.");
        }
        //Appending client ip address
        requestHeader.addHeader("x-forwarded-for", requestHeader.getValueAsString("x-forwarded-for") + "," + clientIPAddress);
    } else {
        // Adding field with value.
        requestHeader.addHeader("x-forwarded-for", clientIPAddress);
    }
}
