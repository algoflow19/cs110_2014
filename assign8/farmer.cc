#include <climits>
#include <getopt.h>
#include <fstream>
#include <set>
#include <socket++/sockstream.h>
#include <thread>
#include <vector>
#include <map>
//#include <algorithm>
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
  // Mutex guarding verifiedTxn vector and file
  mutex verifiedTxnsLock;
  // Mutex guarding outstandingTxn vector
  mutex outstandingTxnsLock;
  // These are outstanding Txns requests that have to be mined and
  // sent to the master.
  vector<Txn> outstandingTxns;

  // This is the master endpoint.  "masterIP" is a dotted quad
  // representation of an IP address.
  string masterIP;
  unsigned short masterPort;

  // All of the following private methods are thread safe
  void farm();
  void connectMaster();
  void handleRequest(int sock);
  void handleTxnRequest(iosockstream& ss);
  void handleBlockRequest(iosockstream& ss);
  void ping(int port, int kPingSleepTime);
  void ingestConnectResponse(iosockstream& ss);
};

// Send a CONNECT request to master.  The response will contain the
// newly verified transactions we've missed since the last time we
// ran.  We store these in verifiedTxns.
void Farmer::connectMaster() {
  // Open sock stream to master
  int sock =  masterConn(masterIP, masterPort);
  sockbuf sb(sock);
  iosockstream ssc(&sb);

  // Send request to master
  sendConnectRequest(ssc, verifiedTxns.size()); 
  // Ingest response
  ingestConnectResponse(ssc);
  //Save Txns
  verifiedTxnsLock.lock();
  writeVerifiedTxns(verifiedTxns);
  verifiedTxnsLock.unlock();
}

// Private Farmer method. Thread safe
// When called as a thread, continuously sends ping with
// port number every kPingSleepTime seconds.
void Farmer::ping(int port, int kPingSleepTime) {
  while (true) {
    // Adding a dummy block to force the compiler
    // to call the destructor of iosockstream
    // and release the socket before going to sleep
    {
      int sock =  masterConn(masterIP, masterPort);
      sockbuf sb(sock);
      iosockstream ssp(&sb);
      sendPingRequest(ssp, port);
    }
    sleep(kPingSleepTime);
  }
}

// Continuously try to farm blocks from outstanding transactions.
// When a rare hash is found, the block is sent to master in a BLOCK
// request. Thread safe.
void Farmer::farm() {
  while (true) {
    // Creating a temp Outstanding Txn vector for building a block.
    vector<Txn> tempOutstandingTxns; 
    // Creating a coinbase and adding it as first Txn
    Txn coin;
    coin.coinbase(this->myName);
    tempOutstandingTxns.push_back(coin);
    // Acquiring lock to extract outstandingTxns vector
    outstandingTxnsLock.lock();
    for (Txn& t : outstandingTxns)
      tempOutstandingTxns.push_back(t);
    outstandingTxnsLock.unlock();

    // Creating a block
    BlockHeader block(tempOutstandingTxns);
    // Check if the block mined has the required hash
    if (block.isRare() == true) {
      // Send mined block to the master to get goodies. 
      int sockf =  masterConn(masterIP, masterPort);
      sockbuf sfb(sockf);
      iosockstream ssf(&sfb);
      sendBlockRequest(ssf, block, tempOutstandingTxns);
    }
  }
}

// Handle a TXN request from a peer.  We read the transactions from
// the request, and store them as outstanding transactions.
// Transactions for coins that already have outstanding transactions
// are not stored, since master would reject those as duplicates.
// Thread safe.
void Farmer::handleTxnRequest(iosockstream& ss) {
  char space;
  int numTxns;
  // Discarding space 
  ss.get(space);
  // Getting num of Txns packed in this message.
  ss >> numTxns;
  if (numTxns == -1) return;
  ss.get(space);
  //Acquiring locks to Manipulate verifiedTxns and outstandingTxns 
  for (int i = 0; i < numTxns; i++) {
    // Reading Txn
    Txn entry;
    ss.read((char *)&entry, sizeof(entry));

    verifiedTxnsLock.lock();
    int verify = entry.verifySig(verifiedTxns);
    verifiedTxnsLock.unlock();

    // Check if Txn is valid
    if (verify == 1 ) {
      bool duplicate = false; 
      outstandingTxnsLock.lock();
      // Check for duplicates
      for (Txn& t : outstandingTxns) {
        if (t.coinID == entry.coinID) {
            duplicate = true;
            break;
        }
      }
      outstandingTxnsLock.unlock();
      if (!duplicate) {
         // Not a duplicate, appending to the outStanding Txn vector
         outstandingTxnsLock.lock();
         outstandingTxns.push_back(entry);
         outstandingTxnsLock.unlock();
      }
    } 
  }
}

// Handle a BLOCK request from master.  Each transaction contained in
// the block is stored in verifiedTxns, which is persisted to disk.
// Outstanding transactions whose signatures no longer validate are
// removed.
// Thread safe.
void Farmer::handleBlockRequest(iosockstream& ss) {
  char space;
  ss.get(space);
  BlockHeader verifiedBlock;
  // Accessing VerifiedTxns after acquiring necessary lock 
  verifiedTxnsLock.lock();
  // Read and save verifiedTxns from the block request
  verifiedBlock.readBlock(ss, verifiedTxns);
  writeVerifiedTxns(verifiedTxns);
  verifiedTxnsLock.unlock();
 
  // Purging Txns from outstandingTxns vector if necessary
  outstandingTxnsLock.lock();
  for (vector<Txn>::iterator it = outstandingTxns.begin(); it!= outstandingTxns.end();) {
    // Acquiring verifiedTxn lock before Txn verification
    verifiedTxnsLock.lock();
    int verify = (*it).verifySig(verifiedTxns);
    verifiedTxnsLock.unlock();
    if (verify != 1) {
      it = outstandingTxns.erase(it);
    } else {
      it++;
    }
  }  
  outstandingTxnsLock.unlock();
}

// Handle a TXN or BLOCK network request.
// Thread safe.
void Farmer::handleRequest(int sock) {
  sockbuf sb(sock);
  iosockstream sshr(&sb);

  int mi;
  sshr >> mi;
  Message m = (Message) mi;

  if (m == TXN) {
    handleTxnRequest(sshr);
  } else if (m == BLOCK) {
    handleBlockRequest(sshr);
  } else {
    cerr << "received unknown message type: " << m << ".  not handling." << endl;
  }

  sshr.flush();
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

  // Spawning ping and farmer threads.
  thread pingThread([this, port, kPingSleepTime] {ping(port, kPingSleepTime);});
  pingThread.detach();
  thread farmThread([this] {farm();});
  farmThread.detach();

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

  verifiedTxnsLock.lock();
  readVerifiedTxns(verifiedTxns);
  verifiedTxnsLock.unlock();
  connectMaster();
}

// Farmer class private method to ingest connect response
// and populate verifiedTxns vector. Thread safe.
void Farmer::ingestConnectResponse(iosockstream& ss) {
  int numNewTxns;
  char space;
  ss >> numNewTxns;
  // Discarding space
  ss.get(space);
  if (numNewTxns == -1) return;
  for (int i = 0; i < numNewTxns; i++) {
    Txn entry;
    ss.read((char *)&entry, sizeof(entry));
    // Adding Txn that as part of connect response
    verifiedTxnsLock.lock();
    verifiedTxns.push_back(entry);
    verifiedTxnsLock.unlock();
  }
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
