/**
 * File: mapreduce-worker.h
 * ------------------------
 * Defines the object type that models a worker
 * within a farm of workers that collectively coordinate
 * with the master server and collectively collaborate
 * to process input files and generate output files.
 *
 * The abstraction is general enough that it can be used
 * to invoke mapper executables (e.g. word-count-mapper)
 * on the original input chunks to generate unsorted files
 * of key/value pairs, or it can be used to invoke reducer
 * executables (e.g. word-count-reducer) to reduce sorted
 * input files of key/vector-of-value pairs down to sorted
 * key/value pairs (where, in the case of the word-count-mapper/
 * word-count-reducer team of executables, the final files are
 * sorted word/frequency tables).
 */

#pragma once
#include <string>
#include <fstream>

class MapReduceWorker {
 public:
  MapReduceWorker(const std::string& serverHost, unsigned short serverPort,
                  const std::string& executablePath, const std::string& executable,
                  const std::string& outputPath);
  void work();
  
 private:
  std::string serverHost;
  unsigned short serverPort;
  std::string executablePath;
  std::string executable;
  std::string outputPath;

  bool requestInputFile(std::string& inputFile);
  void sendProgressReportToServer(const std::string& inputFile);
  int getClientSocket();
  
  MapReduceWorker(const MapReduceWorker& original) = delete;
  void operator=(const MapReduceWorker& rhs) = delete;
};
