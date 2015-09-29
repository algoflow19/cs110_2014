/**
 * File: time-client.cc
 * --------------------
 * Implements a trivially small program that
 * assumes a time server is running on myth22.stanford.edu,
 * port 12345.
 */

#include <iostream>
#include "client-socket.h"
#include "socket++/sockstream.h"
using namespace std;

static const int kTimeServerInaccessible = 1;

int main(int argc, char *argv[]) {
  int clientSocket = createClientSocket("myth22.stanford.edu", 12345);
  if (clientSocket == kClientSocketError) {
    cerr << "Time server could not be reached" << endl;
    cerr << "Aborting" << endl;
    return kTimeServerInaccessible;
  }
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  string timeline;
  getline(ss, timeline);
  cout << timeline << endl;
  return 0;
}
