/**
 * File: hello-server.cc
 * ---------------------
 * Illustrates how multiprocessing (e.g. fork
 */

#include <iostream>                // for cout, cett, endl
#include <ctime>                   // for time, gmtime, strftim
#include <signal.h>                // signal, SIGINT
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "socket++/sockstream.h"   // for sockbuf, iosockstream
#include "server-socket.h"         // for createServerSocket

using namespace std;

static const size_t kNumSlaves = 15;
static pid_t slavePIDs[kNumSlaves];

static void killMasterAndSlaves(int unused) {
  for (size_t i = 0; i < kNumSlaves; i++) {
    cout << "Shutting down server with pid " << slavePIDs[i] << "." << endl;
    kill(slavePIDs[i], SIGINT);
    waitpid(slavePIDs[i], NULL, 0);
  }
  cout << "Shutting down master." << endl;
  exit(0);
}

static void runServer(int server) {
  cout << "Firing up hello server inside process with pid " << getpid() << "." << endl;
  while (true) {
    struct sockaddr_in clientAddress;
    socklen_t length = sizeof(clientAddress);
    int client = accept(server, (struct sockaddr *) &clientAddress, &length);
    cout << "Request from " << inet_ntoa(clientAddress.sin_addr) << " served by server with pid " << getpid() << "." << endl;
    sockbuf sb(client);
    iosockstream ss(&sb);
    ss << "HTTP/1.0 200 OK\r\n";
    ss << "\r\n";
    ss << "Hello from server (pid: " << getpid() << ")" << endl;
  }
}

int main(int argc, char *argv[]) {
  int server = createServerSocket(33333, 128);
  for (size_t i = 0; i < kNumSlaves; i++) {
    slavePIDs[i] = fork();
    if (slavePIDs[i] == 0) runServer(server);
  }
  
  signal(SIGINT, killMasterAndSlaves);
  runServer(server); // let master process be consumed by same server
  return 0;
}
