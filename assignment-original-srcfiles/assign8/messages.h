#pragma once

#include <socket++/sockstream.h>
#include <vector>

#include "txn.h"
#include "block.h"
#include "endpoint.h"

const static std::string kRemoteMasterHost = "171.64.15.178";
const static int kRemoteMasterPort = 8333;
const static int kMaxBacklog = 128;

enum Message {
  // CONNECT
  //   sent from farmer->master or ./send->master on boot to retrieve
  //   newly verified transactions.
  //
  //   Request:
  //     "<CONNECT> <numVerifiedTxns>\n"
  //     numVerifiedTxns: number of verified transactions the farmer currently knows about
  //
  //   Response:
  //     "<numNewTxns> [<Txns>]"
  //     numNewTxns: length of [<Txns>].  -1 if you provided an
  //                 invalid numVerifiedTxns.  In this case, no Txns
  //                 are sent.
  //     Txns: list of serialized Txn objects
  CONNECT,

  // PEERS
  //   sent from ./send->master to retrieve the currently active farmers
  //
  //   Request:
  //     "<PEERS>\n"
  //
  //   Response:
  //     "<numPeers> [<Endpoints>]"
  //     numPeers: length of [<Endpoints>]
  //     Endpoints: list of serialized Endpoint objects
  PEERS,

  // PING
  //   sent from farmer->master periodically to let the master know
  //   that the farmer is still active.
  //
  //   Request:
  //     "<PING> <port>\n"
  //     port: port farmer is listening on
  //
  //   Response
  //     None
  PING,

  // BLOCK
  //   sent from farmer->master when a new block is farmed, or from
  //   master->farmer when the txns in that block should now be
  //   considered verified.
  //
  //   Request:
  //     "<BLOCK> <BlockHeader>[<Txns>]"
  //     BlockHeader: serialized BlockHeader object
  //     Txns: list of serialized Txn objects
  //
  //   Response:
  //     None
  BLOCK,

  // TXN
  //   sent from ./send->farmer
  //
  //   Request:
  //     "<TXN> [numTxns] [<Txns>]"
  //     numTxns: number of Txn objects
  //     Txns: list of serialized Txn objects.  one for each of the
  //           coins being transacted.
  //
  //   Response:
  //     None
  TXN
};

void sendPeersRequest(iosockstream& ss);
void sendPingRequest(iosockstream& ss, unsigned short port);
void sendBlockRequest(iosockstream& ss, BlockHeader& bh, std::vector<Txn>& txns);
void sendTxnRequest(iosockstream& ss, std::vector<Txn>& txns);
void sendConnectRequest(iosockstream& ss, int numVerifiedTxns);
int masterConn(std::string masterIP, unsigned short masterPort);
