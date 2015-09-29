#include <climits>
#include <getopt.h>
#include <fstream>
#include <set>
#include <socket++/sockstream.h>
#include <thread>
#include <vector>

#include "block.h"
#include "exit-utils.h"
#include "messages.h"
#include "server-socket.h"
#include "txn.h"
#include "thread-pool.h"
#include "utils.h"

using namespace std;

const int kPingSleepTime = 5;

class Farmer {
public:
  Farmer(string privKeyFname,
         string masterIP,
         unsigned short masterPort);
  void serve();

private:
   // The farmer's private key
  RSA* privKey;
  string myName;

  // These are verified transactions we've received from a BLOCK
  // request from master.  They represent coin ownership, and are
  // persisted to disk via readVerifiedTxns() and writeVerifiedTxns().
  vector<Txn> verifiedTxns;
  mutex verifiedTxnsLock;

  // This is the master endpoint.  "masterIP" is a dotted quad
  // representation of an IP address.
  string masterIP;
  unsigned short masterPort;

  void farm();
  void connectMaster();
  void handleRequest(int sock);
  void handleTxnRequest(iosockstream& ss);
  void handleBlockRequest(iosockstream& ss);
};

// Send a CONNECT request to master.  The response will contain the
// newly verified transactions we've missed since the last time we
// ran.  We store these in verifiedTxns.
void Farmer::connectMaster() {
  // TODO: Step 1
}

// Continuously try to farm blocks from outstanding transactions.
// When a rare hash is found, the block is sent to master in a BLOCK
// request.
void Farmer::farm() {
  // TODO: Step 4
}

// Handle a TXN request from a peer.  We read the transactions from
// the request, and store them as outstanding transactions.
// Transactions for coins that already have outstanding transactions
// are not stored, since master would reject those as duplicates.
void Farmer::handleTxnRequest(iosockstream& ss) {
  // TODO: Step 3
}

// Handle a BLOCK request from master.  Each transaction contained in
// the block is stored in verifiedTxns, which is persisted to disk.
// Outstanding transactions whose signatures no longer validate are
// removed.
void Farmer::handleBlockRequest(iosockstream& ss) {
  // TODO : Step 5
}

// Handle a TXN or BLOCK network request.
void Farmer::handleRequest(int sock) {
  sockbuf sb(sock);
  iosockstream ss(&sb);

  int mi;
  ss >> mi;
  Message m = (Message) mi;

  if (m == TXN) {
    handleTxnRequest(ss);
  } else if (m == BLOCK) {
    handleBlockRequest(ss);
  } else {
    cerr << "received unknown message type: " << m << ".  not handling." << endl;
  }

  ss.flush();
}

// Start a server listening on a random port.  Before we start
// accepting connections, we start a thread that pings master and a
// thread that farms blocks.  New connections are handled
// asynchronously by a thread pool.
static const int kNumReqThreads = 20;
void Farmer::serve() {
  unsigned short port;
  int serverSock;
  for (int i = 0; i < 3; i++) {
    port = randPort();
    serverSock = createServerSocket(port, kMaxBacklog);
    if (serverSock >= 0) {
      break;
    }
  }
  exitIf(serverSock < 0, 1, stderr, "server socket error.  exiting.\n");

  log("farming server starting on port %d...", port);

  // TODO: Step 2

  thread farmThread([this] {farm();});

  ThreadPool reqPool(kNumReqThreads);
  while (true) {
    struct sockaddr_in ip;
    socklen_t addrlen = sizeof(ip);
    int sock = accept(serverSock, (struct sockaddr *) &ip, &addrlen);
    exitIf(sock == -1, 1, stderr, "accept error\n");

    reqPool.schedule([this, sock] {handleRequest(sock);});
  }
}

// Farmer constructor.  Read the private key from disk, read the
// verifiedTxns vector from disk, and send a CONNECT request to
// master.
Farmer::Farmer(string privKeyFname,
               string masterIP,
               unsigned short masterPort) {
  privKey = readPrivKey(privKeyFname);
  exitIf(privKey == NULL, 1, stderr,
         "error reading private key from file: %s",
         privKeyFname.c_str());

  myName = getUser(privKey);
  exitIf(myName == "", 1, stderr, "Unknown public key");

  this->masterIP = masterIP;
  this->masterPort = masterPort;

  readVerifiedTxns(verifiedTxns);
  connectMaster();
}

static const string kUsageString = string("./farmer -h\n") +
  "./farmer [-g] [-v]\n" +
  "\n" +
  "options:\n" +
  "  -h    print this help message\n"
  "  -g    connect to global network\n"
  "  -v    verbose\n";
int main(int argc, char **argv) {
  // ensure farmers that start at the same time do not get the same seed
  srand(time(NULL) ^ getpid());

  while (true) {
    int ch = getopt(argc, argv, "hgv");
    if (ch == -1) break;
    switch(ch) {
    case 'h':
      cout << kUsageString << endl;
      exit(0);
    case 'g':
      global = true;
      break;
    case 'v':
      verbose = true;
      break;
    default:
      cerr << kUsageString << endl;
      exit(1);
    }
  }

  parseUsers();

  string masterIP;
  unsigned short masterPort;
  if (global) {
    masterIP = kRemoteMasterHost;
    masterPort = kRemoteMasterPort;
  } else {
    masterIP = "127.0.0.1";
    masterPort = readPortFile();
  }

  Farmer farmer(kPrivKeyFname, masterIP, masterPort);
  farmer.serve();
}
