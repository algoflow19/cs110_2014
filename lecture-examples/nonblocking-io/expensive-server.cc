/**
 * File: outbound-file-server.cc
 * -----------------------------
 * Illustrates how nonblocking IO can be used to implement
 * an single-thread web server.  This particular example wastes
 * a huge amount of CPU time (as it while loops forever without
 * blocking), but it does demonstrate how nonblocking IO can
 * be used to very easily serve multiple client requests as a
 * time without using threading.
 */

#include "outbound-file.h"
#include "server-socket.h"
#include "non-blocking-utils.h"
#include <sys/socket.h> // for accept
#include <list>
#include <cassert>
#include <iostream>
using namespace std;

static const unsigned short kDefaultPort = 12345;
static const int kDefaultBacklog = 128;
static const string kFileToServe("expensive-server.cc.txt");
int main(int argc, char **argv) {
  int serverSocket = createServerSocket(kDefaultPort, kDefaultBacklog);
  if (serverSocket == kServerSocketFailure) {
    cerr << "Could not start server.  Port " << kDefaultPort << " is probably in use." << endl;
    return 0;
  }

  setAsNonBlocking(serverSocket);
  cout << "Static file server listening on port " << kDefaultPort << "." << endl;  
  list<OutboundFile> outboundFiles;
  size_t numConnections = 0;
  size_t numActiveConnections = 0;
  
  while (true) {
    int clientSocket = accept(serverSocket, NULL, NULL);
    if (clientSocket == -1) {
      assert(errno == EWOULDBLOCK);
    } else {
      OutboundFile obf;
      obf.initialize(kFileToServe, clientSocket);
      outboundFiles.push_back(obf);
      cout << "Connection #" << ++numConnections << endl;
      cout << "Queue size: " << ++numActiveConnections << endl;
    }
    
    auto iter = outboundFiles.begin();
    while (iter != outboundFiles.end()) {
      if (iter->sendMoreData()) {
        ++iter;
      } else {
        iter = outboundFiles.erase(iter);
        cout << "Queue size: " << --numActiveConnections << endl;
      }
    }
  }
}
