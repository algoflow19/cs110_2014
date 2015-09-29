/**
 * File: mapreduce-worker.cc
 * -------------------------
 * Presents the implementation of the MapReduceWorker class.
 */

#include "mapreduce-worker.h"
#include <cassert>
#include <sstream>
#include "mr-messages.h"
#include "string-utils.h"
#include "client-socket.h"
#include "socket++/sockstream.h"
using namespace std;

MapReduceWorker::MapReduceWorker(const string& serverHost, unsigned short serverPort,
                                 const string& executablePath, const string& executable,
                                 const string& outputPath) :
  serverHost(serverHost), serverPort(serverPort), executablePath(executablePath), 
  executable(executable), outputPath(outputPath) {}

void MapReduceWorker::work() {
  while (true) {
    string inputFile;
    if (!requestInputFile(inputFile)) break;
    sendProgressReportToServer(inputFile);
  }
}

bool MapReduceWorker::requestInputFile(string& inputFile) {
  int clientSocket = getClientSocket();
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  sendWorkerReady(ss);
  MRMessage message;
  string payload;
  receiveMessage(ss, message, payload);
  if (message == kServerDone) return false;
  inputFile = trim(payload);
  return true;
}

void MapReduceWorker::sendProgressReportToServer(const string& inputFile) {
  int clientSocket = getClientSocket();
  sockbuf sb(clientSocket);
  iosockstream ss(&sb);
  sendJobSucceeded(ss, inputFile);
}

static const int kServerInaccessible = 2;
int MapReduceWorker::getClientSocket() {
  int clientSocket = createClientSocket(serverHost, serverPort);
  if (clientSocket == kClientSocketError) {
    exit(kServerInaccessible);
  }
  
  return clientSocket;
}
