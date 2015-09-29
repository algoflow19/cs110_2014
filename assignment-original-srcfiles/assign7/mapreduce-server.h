/**
 * File: mapreduce-server.h
 * ------------------------
 * Models the master node in the entire MapReduce system.
 */

#pragma once
#include <cstdlib>
#include <string>
#include <thread>
#include <list>
#include <map>
#include "mapreduce-server-exception.h"

class MapReduceServer {
 public:
  MapReduceServer(int argc, char **argv) throw (MapReduceServerException);
  ~MapReduceServer() throw();
  void run() throw();
  
 private:
  unsigned short computeDefaultPortForUser();
  void ensureEnvironmentVariablesPresent() throw (MapReduceServerException);
  void parseArgumentList(int argc, char **argv) throw (MapReduceServerException);
  unsigned short extractPortNumber(const char *portArgument) throw (MapReduceServerException);
  void initializeFromConfigFile(const std::string& configFileName) throw (MapReduceServerException);
  void applyToServer(const std::string& key, const std::string& value) throw (MapReduceServerException);
  std::string ensureDirectoryExists(const std::string& key, const std::string& path) throw (MapReduceServerException);
  size_t parseNumberInRange(const std::string& key, const std::string& value, 
                            size_t low, size_t high) throw (MapReduceServerException);
  void buildIPAddressMap() throw();
  void stageInputFiles(const std::string& inputDirectory) throw();
  void startServer() throw (MapReduceServerException);
  void logServerConfiguration(std::ostream& os) throw();
  void orchestrateWorkers() throw();
  void handleRequest(int clientSocket, const std::string& clientIPAddress) throw();
  void spawnMappers() throw();
  void spawnReducers() throw();
  std::string buildCommand(const std::string& workerHost, const std::string& executablePath, 
                           const std::string& executable, const std::string& outputPath) throw();
  void spawnWorker(const std::string& node, const std::string& command) throw();
  void groupByKey() throw();
  bool surfaceNextChunk(std::string& chunkName) throw();
  void markChunkAsProcessed(const std::string& clientIPAddress, const std::string& chunkName) throw();
  void rescheduleChunk(const std::string& clientIPAddress, const std::string& chunkName) throw();
  void bringDownServer() throw();

  int serverSocket;
  unsigned short serverPort;
  size_t numMappers;
  size_t numReducers;
  std::string executablePath;
  std::string mapInputPath;
  std::string mapOutputPath;
  std::string reduceInputPath;
  std::string reduceOutputPath;
  std::string mapperExecutable;
  std::string reducerExecutable;

  std::map<std::string, std::string> ipAddressMap;
  bool serverIsRunning; // only manipulated in constructor and in server thread, so no lock needed
  std::thread serverThread;
  
  std::list<std::string> unprocessedChunks;
  
  MapReduceServer(const MapReduceServer& original) = delete;
  MapReduceServer& operator=(const MapReduceServer& rhs) = delete;
};
