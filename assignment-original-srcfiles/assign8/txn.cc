#include <cstring>

#include "key.h"
#include "txn.h"
#include "utils.h"

using namespace std;

Txn::Txn(string toName, coin_id_t coinID, RSA* sigKey) {
  strncpy(this->toName, toName.c_str(), kNameMaxLen);
  this->coinID = coinID;
  if (sigKey != NULL) {
    this->sign(sigKey);
  }
}

// Convert this Txn object into a "coinbase".  A coinbase is the first
// transaction in a block.  It is meant to award the farmer a new
// coin.  It uses the farmer's public key as the recipient key, a
// random coin id for the coin, and an empty signature.
void Txn::coinbase(string toName) {
  strncpy(this->toName, toName.c_str(), kNameMaxLen);
  memset(this->fromSig, 0, kSigLen);
  this->coinID = lrand();
}

// Create a copy of this transaction with the signature field set to
// zero.  This is used by sign().
void Txn::zeroed(Txn& t) {
  memcpy(&t, this, sizeof(t));
  memset(&t.fromSig, 0, kSigLen);
}

// Sign a Txn object.
//   key: the sender's private key
void Txn::sign(RSA* key) {
  Txn t;
  zeroed(t);

  RSASign(key, &t, sizeof(t), this->fromSig);
}

// Verify the Txn object's signature.  Returns:
//   1: success
//   0: invalid signature
//   -1: no previous transaction for this coins
//   -2: recipient of transaction is unknown
//
// A sender signs a transaction by calling sign() on the object.  It
// then sends the Txn across the network via a TXN request.  The
// recipients then verify the Txn's authenticity by calling
// verifySig(), which uses the the sender's public key to verify the
// signature.  We find the public key by looking at the public key of
// the last verified transaction for this coin.
int Txn::verifySig(vector<Txn>& verifiedTxns) {
  Txn last;
  if (!lastTxn(verifiedTxns, coinID, last)) {
    return -1;
  } else {
    Txn t;
    zeroed(t);

    string toName(last.toName);
    RSA* toPubKey = getKey(toName);
    if (toPubKey == NULL) {
      return -2;
    }
    return RSAVerify(toPubKey, &t, sizeof(t), this->fromSig);
  }
}

// bool operator < (const Txn& t1, const Txn& t2) {
//   return memcmp(&t1, &t2, sizeof(Txn)) < 0;
// }
