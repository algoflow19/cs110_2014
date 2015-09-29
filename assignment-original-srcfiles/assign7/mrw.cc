/**
 * File: mrw.cc
 * ------------
 * This provides the entry point for all worker clients.
 * The implementation is straightforward, as it passes the
 * buck to the MapReduceWorker class.  See mapreduce-worker.h/cc
 * for the full details on how the workers do their job.
 */

#include "mapreduce-worker.h"

static const int kExpectedArgumentCount = 6;
static const int kWrongArgumentCountError = 99;
int main(int argc, char *argv[]) {
  if (argc != kExpectedArgumentCount) return kWrongArgumentCountError;
  MapReduceWorker mrw(/* serverHost = */ argv[1], 
                      /* serverPort = */ atoi(argv[2]), 
                      /* executablePath = */ argv[3], 
                      /* executable = */ argv[4], 
                      /* outputPath = */ argv[5]);
  mrw.work();
  return 0;
}
