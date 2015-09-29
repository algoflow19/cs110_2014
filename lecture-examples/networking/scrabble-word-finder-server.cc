/**
 * File: scrabble-word-finder-server.cc
 * ------------------------------------
 * This is a server, much like time-server-concurrennt is, except that
 * the payload is synthesized by another executable, whose output is caught
 * because open is used to catch its output.
 */

#include <string>
#include <vector>
#include <map>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server-socket.h"
#include "scrabble-word-finder.h"
#include "socket++/sockstream.h"
using namespace std;

/**
 * Function: extractPort
 * ---------------------
 * Accepts the specified string, presumably a numeric one,
 * converts it to an unsigned short, and returns it.  
 * If the numeric string is malformed, or if the unsigned 
 * short is out of range, then USHRT_MAX is returned as a sentinel.
 */

static const unsigned short kDefaultPort = 13133;
static const unsigned short kIllegalPort = USHRT_MAX;
static unsigned short extractPort(const char *portString) {
  if (portString == NULL) return kDefaultPort;
  if (portString[0] == '\0') return kIllegalPort;
  
  char *end = NULL;
  long port = strtol(portString, &end, 0);
  if (end[0] != '\0') return kIllegalPort;
  if (port < 1024 && port >= USHRT_MAX) return kIllegalPort;
  return (unsigned short) port;
}

/**
 * Function: pullFormableWords
 * ---------------------------
 * Ingests the entire subprocess's output (which is out input) from the
 * provided FILE *, and populates the reference vector with all of the
 * process's output (each vector entry is one line)
 */
static void pullFormableWords(vector<string>& formableWords, FILE *infileFromProcess) {
  while (true) {
    char word[64] = {'\0'};
    if (fscanf(infileFromProcess, "%63[^\n]\n", word) != 1) return;
    formableWords.push_back(word);
  }
}

/**
 * Function: getLetters
 * --------------------
 * Retrieved the set of letters from the GET request.
 * We could use the HTTPRequest class from the http-proxy assignment,
 * but I instead just ingest everything manually, using subparts of
 * the HTTPRequest class implementation.
 */
static string getLetters(iosockstream& ss) {
  string method, path, protocol;
  ss >> method >> path >> protocol;
  if (ss.fail()) return ""; // in case request isn't HTTP
  string rest;
  getline(ss, rest);
  size_t pos = path.rfind("/");
  return pos == string::npos ? path : path.substr(pos + 1);
}

/**
 * Function: skipHeaders
 * ---------------------
 * Reads everything up through and including the first blank line from
 * the incoming HTTP request layered underneath the provided iosockstream.
 */
static void skipHeaders(iosockstream& ss) {
  string line;
  do {
    getline(ss, line);
  } while (!line.empty() && line != "\r");
}

/**
 * Function: constructJSONArray
 * ----------------------------
 * Presents a JSON serialization of the provided vector (aka array).  If
 * the vector is empty or of size one, then no newlines are inserted.  But if
 * it's of length 2 or more, then the array serialization is spread over many
 * lines (one array entry per line) and the indent parameter is used to clarify
 * how far each shoudl be indented.
 */
static string constructJSONArray(const vector<string>& possibilities, int indent) {
  if (possibilities.empty()) return "[]";
  if (possibilities.size() == 1) return "[" + possibilities.front() + "]";

  bool first = true;
  string jsonArray = "[";
  for (const string& possibility: possibilities) {
    if (!first) {
      jsonArray += ", ";
    } else {
      first = false;
    }

    jsonArray += "\n";
    for (int i = 0; i < indent; i++) jsonArray += "  ";
    jsonArray += ("'" + possibility + "'");
  }
  
  jsonArray += "\n";
  for (int i = 0; i < indent - 1; i++) jsonArray += "  ";
  jsonArray += "]";
  return jsonArray;
}

/**
 * Function: constructPayload
 * --------------------------
 * Constructs the payload string by publishing a small JS object literal to
 * the supplied ostringstream.  The JS object includes a success Boolean,
 * a status message (regardless of success status), and if the request was
 * well-formed, a JS array (in string form) of all of the different words
 * that can be formed.
 */
static void constructPayload(int status, const vector<string>& formableWords, 
			     bool cached, double time, ostringstream& payload) {
  payload << "{" << endl;
  bool success = status == 0;
  payload << "  success: " << boolalpha << success << "," << endl;
  payload << "  time: " << time << "," << endl;
  if (success) {
    payload << "  cached: " << boolalpha << cached << "," << endl;
    payload << "  possibilities: " << constructJSONArray(formableWords, 2) << endl;
  } else {
    payload << "  message: '" << kErrorMessages[WEXITSTATUS(status)] << endl;
  }
  payload << "}" << endl;  
}

/**
 * Function: sendResponse
 * ----------------------
 * Constructs the smallest possible HTTP response out of the specified
 * payload.  The understanding is that the content type is JavaScript, and
 * we assume that all requests are identified as successful, even if the payload
 * reports that the request was borked or malformed.  (In practice, the
 * response would be much more informative that it is, but this example is more
 * about popen and pclose than it is about HTTP responses).
 */
static void sendResponse(iosockstream& ss, const string& payload) {
  ss << "HTTP/1.1 200 OK\r\n";
  ss << "Content-Type: application/javascript; charset=UTF-8\r\n";
  ss << "Content-Length: " << payload.size() << "\r\n";
  ss << "\r\n";
  ss << payload;
  ss << endl << flush;
}

/**
 * Function: publishScrabbleWords
 * ------------------------------
 * Receives the HTTP request for a list of formable Scrabble words,
 * where the rack of letters is encoded into the path of the request,
 * and publishes the response after the response has been constructed.
 *
 * The new hotness here is that popen and pclose are used to invoke 
 * a subprocess.
 */
static mutex m;
static map<string, vector<string>> cache;
static void publishScrabbleWords(int clientSocket) {
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  string letters = getLetters(ss);
  sort(letters.begin(), letters.end());
  skipHeaders(ss);
  struct timeval start, end, duration;
  m.lock();
  int status = 0;
  gettimeofday(&start, NULL);
  vector<string> formableWords;
  auto found = cache.find(letters);
  bool cached = found != cache.end();
  if (cached) {
    m.unlock();
    formableWords = found->second;
  } else {
    string command = "./scrabble-word-finder \"" + letters + "\"";
    FILE *infileFromProcess = popen(command.c_str(), "r");
    pullFormableWords(formableWords, infileFromProcess);
    status = pclose(infileFromProcess);
    if (status == 0) cache[letters] = formableWords;
    m.unlock();
  }
  gettimeofday(&end, NULL);
  timersub(&end, &start, &duration);
  double time = duration.tv_sec + duration.tv_usec/1000000.0;
  ostringstream payload;
  constructPayload(status, formableWords, cached, time, payload);
  sendResponse(ss, payload.str());
}

/**
 * Function: main
 * --------------
 * Defines the entry point for the server, which relies on the presence of
 * the scrabble-word-finder executable in the same directory as this server
 * executable.
 */
static const int kWrongArgumentCount = 1;
static const int kIllegalPortArgument = 2;
static const int kServerStartFailure = 3;
int main(int argc, char *argv[]) {
  if (argc > 2) {
    cerr << "Usage: " << argv[0] << " [<port>]" << endl;
    return kWrongArgumentCount;
  }
  
  unsigned short port = extractPort(argv[1]);
  if (port == kIllegalPort) {
    cerr << "Error: Argument must be purely numeric " 
	 << "and within range [1024, " << kIllegalPort << ")" << endl;
    cerr << "Aborting... " << endl;
    return kIllegalPortArgument;
  }
  
  static const int kMaxQueueSize = 10;
  int serverSocket = createServerSocket(port, kMaxQueueSize);
  if (serverSocket == kServerSocketFailure) {
    cerr << "Error: Could not start time server to listen to port " << port << "." << endl;
    cerr << "Aborting... " << endl;
    return kServerStartFailure;
  }
  
  cout << "Server listening on port " << port << "." << endl;
  while (true) {
    struct sockaddr_in clientAddress;
    socklen_t clientAddressSize = sizeof(clientAddress);
    memset(&clientAddress, 0, clientAddressSize);
    int clientSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &clientAddressSize);
    cout << "Received a connection request from " 
	 << inet_ntoa(clientAddress.sin_addr) << "." << endl;
    thread t(publishScrabbleWords, clientSocket);
    t.detach();
  }
  
  return 0;
}
