#include <arpa/inet.h>
#include <vector>

#include "client-socket.h"
#include "exit-utils.h"
#include "messages.h"
#include "utils.h"

using namespace std;

void sendPingRequest(iosockstream& ss, unsigned short port) {
  log("sending PING request");
  ss << PING << ' ' << port << endl << flush;
}

void sendBlockRequest(iosockstream& ss, BlockHeader& bh, vector<Txn>& txns) {
  log("sending BLOCK request with %d txns.", txns.size());
  ss << BLOCK << ' ';

  ss.write((char*) &bh, sizeof(bh));

  for (Txn& t : txns) {
    ss.write((char*) &t, sizeof(t));
  }
  ss.flush();
}

void sendTxnRequest(iosockstream& ss, vector<Txn>& txns) {
  log("sending TXN request");
  ss << TXN << ' ' << txns.size() << ' ';

  for (Txn& t : txns) {
    ss.write((char*) &t, sizeof(t));
  }
  ss.flush();
}

void sendConnectRequest(iosockstream& ss, int numVerifiedTxns) {
  log("sending CONNECT request");
  ss << CONNECT << ' ' << numVerifiedTxns << endl << flush;
}

void sendPeersRequest(iosockstream& ss) {
  log("sending PEERS request");
  ss << PEERS << endl << flush;
}

int masterConn(string masterIP, unsigned short masterPort) {
  struct in_addr in;
  inet_aton(masterIP.c_str(), &in);

  int sock = createClientSocket(&in, masterPort);
  exitIf(sock == kClientSocketError, 1, stderr,
         "master node %s:%d down\n",
         masterIP.c_str(), masterPort);
  return sock;
}
