#pragma once

#include <vector>
#include <string>

#include "key.h"

typedef unsigned long long coin_id_t;
const int kNameMaxLen = 32;

// A Txn (transaction) represents a coin's transfer of ownership from one user to another.
//   toName: The receiving user's name
//   coinID: The ID of the coin
//   fromSig: The sender's signature of the Txn object.  The signature
//            is computed with fromSig zeroed out
class Txn {
 public:
  char toName[kNameMaxLen];
  coin_id_t coinID;
  unsigned char fromSig[kSigLen];

  Txn() {};
  Txn(std::string toName, coin_id_t coinID, RSA* sigKey = NULL);

  void coinbase(std::string name);
  void zeroed(Txn& t);
  void sign(RSA* key);
  int verifySig(std::vector<Txn>& verifiedTxns);

  //friend bool operator < (const Txn& e1, const Txn& e2);
};
