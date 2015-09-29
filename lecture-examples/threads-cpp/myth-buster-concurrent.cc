/**
 * File: myth-buster-concurrent.cc
 * -------------------------------
 * Presents a multi-threaded version of the same exact program presented
 * in myth-buster-sequential.cc
 */

#include <iostream>
#include <fstream>
#include <unordered_set>
#include <map>
#include <vector>
#include <algorithm>
#include <thread>
#include <mutex>
#include "myth-buster-service.h"
#include "ostreamlock.h"
#include "semaphore.h"
#include "string-utils.h"
using namespace std;

static unordered_set<string> sunetIDs;
static map<unsigned short, unsigned short> processCountMap;
static mutex processCountMapLock;

static const int kFileInaccessible = 1;
static void readStudentFile(const string& filename) {
  ifstream infile(filename.c_str());
  if (infile.fail()) {
    cerr << "CS110 Student SUNet ID list not found." << endl;
    exit(kFileInaccessible);
  }
  
  while (true) {
    string sunetID;
    getline(infile, sunetID);
    if (infile.fail()) return;
    sunetID = trim(sunetID);
    sunetIDs.insert(sunetID);
  }
}

static void countCS110Processes(unsigned short num, semaphore& s) {
  int numProcesses = getNumProcesses(num, sunetIDs);
  if (numProcesses >= 0) {
    processCountMapLock.lock();
    processCountMap[num] = numProcesses;
    processCountMapLock.unlock();
    cout << "myth" << num << " has this many CS110-student processes: " << numProcesses << endl;
  }

  s.signal(on_thread_exit);
}

static unsigned short kMinMythMachine = 1;
static unsigned short kMaxMythMachine = 30;
static int kMaxNumThreads = 12; // really maximum number of threads doing meaningful work
static void compileCS110ProcessCountMap() {  
  vector<thread> threads;
  semaphore numAllowed(kMaxNumThreads);
  for (unsigned short num = kMinMythMachine; num <= kMaxMythMachine; num++) {
    numAllowed.wait();
    threads.push_back(thread(countCS110Processes, num, ref(numAllowed)));
  }
  
  for (thread& t: threads) t.join();
}

static bool isLessLoaded(const pair<unsigned short, unsigned short>& one,
			 const pair<unsigned short, unsigned short>& two) {
  return one.second < two.second || (one.second == two.second && one.first > two.first);
}

static void publishLeastLoadedMachineInfo() {
  auto leastLoaded = min_element(processCountMap.cbegin(), processCountMap.cend(), isLessLoaded);
  cout << "Machine least loaded by CS110 students: myth" << leastLoaded->first << endl;
  cout << "Number of CS110 processes on least loaded machine: " << leastLoaded->second << endl;
}

static const char *kCS110StudentIDsFile = "studentsunets.txt";
int main(int argc, char *argv[]) {
  readStudentFile(argv[1] != NULL ? argv[1] : kCS110StudentIDsFile);
  compileCS110ProcessCountMap();
  publishLeastLoadedMachineInfo();
  return 0;
}
