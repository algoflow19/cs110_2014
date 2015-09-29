#include <climits>
#include <cstdarg>
#include <cstring>
#include <fstream>
#include <iostream>
#include <cassert>
#include <sys/time.h>
#include <set>

#include "utils.h"
#include "ostreamlock.h"

using namespace std;

int verbose = false;
bool global = false;

map<string, RSA*> userKeys;

const string kLocalVerifiedTxns = "verified_txns";
const string kGlobalVerifiedTxns = "verified_txns_global";

string masterPortFileName() {
  return string("/tmp/") + getenv("USER") + "-master-port";
}

unsigned long long shortrand() {
  return (rand() & ((2<<16) - 1));
}

unsigned long long lrand() {
  return (shortrand() << 48) ^ (shortrand() << 32) ^ (shortrand() << 16) ^ shortrand();
}

const unsigned short kLowestPortNumber = 1024;
unsigned short randPort() {
  return rand() % (USHRT_MAX - kLowestPortNumber) + kLowestPortNumber;
}

void baseLog(const char *msg) {
  struct timeval tv;
  gettimeofday(&tv, NULL);

  cout << oslock;
  printf("[%ld.%.6ld] %s\n", tv.tv_sec, tv.tv_usec, msg);
  fflush(stdout);
  cout << osunlock;
}

void vlog(const char *control, ...) {
  if (!verbose) return;

  char buf[1024];

  va_list arglist;
  va_start(arglist, control);
  vsnprintf(buf, 1024, control, arglist);
  va_end(arglist);

  baseLog(buf);
}

void log(const char *control, ...) {
  char buf[1024];

  va_list arglist;
  va_start(arglist, control);
  vsnprintf(buf, 1024, control, arglist);
  va_end(arglist);

  baseLog(buf);
}

// Find the last verified Txn for a given coin.  This is used to find
// the public key of the previous owner, so we can validate the
// signature of a new Txn.
int lastTxn(vector<Txn>& verifiedTxns, coin_id_t coinID, Txn& last) {
  for (int i = verifiedTxns.size() - 1; i >= 0; i--) {
    Txn& t = verifiedTxns[i];

    if (t.coinID == coinID) {
      last = t;
      return 1;
    }
  }
  return 0;
}

// Save to disk the Txn objects stored in "verifiedTxns".
void writeVerifiedTxns(vector<Txn>& verifiedTxns) {
  string verifiedTxnFileName = global ? kGlobalVerifiedTxns : kLocalVerifiedTxns;
  FILE* f = fopen(verifiedTxnFileName.c_str(), "w");
  if (f == NULL) {
    log("error opening file: %s", verifiedTxnFileName.c_str());
    return;
  }
  for (Txn& t : verifiedTxns) {
    fwrite(&t, sizeof(t), 1, f);
  }
  fclose(f);

  vlog("Saved %zu verified txns to %s", verifiedTxns.size(), verifiedTxnFileName.c_str());
}

// Read Txn objects from disk and store them in "verifiedTxns".
void readVerifiedTxns(vector<Txn>& verifiedTxns) {
  string verifiedTxnFileName = global ? kGlobalVerifiedTxns : kLocalVerifiedTxns;
  verifiedTxns.clear();

  Txn t;
  FILE* f = fopen(verifiedTxnFileName.c_str(), "a+");
  if (f == NULL) {
    log("error opening file: %s", verifiedTxnFileName.c_str());
    return;
  }
  while(fread(&t, sizeof(t), 1, f) == 1) {
    verifiedTxns.push_back(t);
  }
  fclose(f);

  vlog("Loaded %zu verified txns from %s.", verifiedTxns.size(), verifiedTxnFileName.c_str());
}

// A locally running master must listen on a random port.  In order to
// communicate that port to the farmers, it stores it in a temporary
// file.  This function is called by the farmers to read and return
// that port.
unsigned short readPortFile() {
  ifstream portFile;
  portFile.open(masterPortFileName());
  if (portFile.fail()) {
    cerr << "error opening file: " << masterPortFileName()
         << ". Is the local master running?" << endl;
    exit(1);
  }

  unsigned short port;
  portFile >> port;
  portFile.close();
  return port;
}

// Parses a user file into a map of names to RSA public keys.  This
// map is used to lookup the public keys belonging to the names stored
// in Txn objects.
static const string kLocalUserFile = "localusers";
static const string kGlobalUserFile = "globalusers";
void parseUsers() {
  string filename = global ? kGlobalUserFile : kLocalUserFile;
  ifstream users;
  users.open(filename);
  if (users.fail()) {
    cerr << "error opening file: " << filename << endl;
    exit(1);
  }

  while(true) {
    string name;
    users >> name;
    if (users.fail()) break;

    users.get(); // newline
    char toPubKey[kPubKeyLen];
    users.read(toPubKey, kPubKeyLen);

    RSA* pubKey = rsaPubKey(toPubKey);
    if (pubKey == NULL) {
      cerr << "invalid pubkey in users file for user: " << name << endl;
      exit(1);
    } else {
      userKeys[name] = pubKey;
    }
  }
}

string getUser(RSA *pubKey) {
  char *ckey = charPubKey(pubKey);
  if (ckey == NULL) return "";

  for (pair<string, RSA*> elem : userKeys) {
    char* c = charPubKey(elem.second);
    assert(c != NULL);

    if (memcmp(c, ckey, kPubKeyLen) == 0) {
      return elem.first;
    }
  }

  return "";
}

RSA* getKey(string user) {
  if (userKeys.find(user) != userKeys.end()) {
    return userKeys[user];
  } else {
    cerr << "Unknown user: " << user << endl;
    return NULL;
  }
}
