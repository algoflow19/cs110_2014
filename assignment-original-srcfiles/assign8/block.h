#pragma once

#include <vector>
#include <openssl/sha.h>
#include "txn.h"

// A BlockHeader stores information about the Txn objects in a block.
// It is sent along with these Txn objects in a BLOCK request.
//   numTxns: number of Txns in the block
//   rand: random value computed while farming
//   hash: hash of the block, with the hash field set to zero
class BlockHeader {
 public:
  int numTxns;
  unsigned long long rand;
  unsigned char hash[SHA_DIGEST_LENGTH];

  BlockHeader();
  BlockHeader(std::vector<Txn>& txns);

  void readBlock(std::istream& ss, std::vector<Txn>& txns);
  void hashBlock(std::vector<Txn>& txns);
  int verifyBlock(std::vector<Txn>& txns, std::vector<Txn>& verifiedTxns, BlockHeader& cpy);
  bool isRare();
};
