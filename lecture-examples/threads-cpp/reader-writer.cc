/**
 * Implements the classic reader-writer thread example, where
 * one thread writes to a shared buffer and a second thread reads
 * from it.
 */

#include <mutex>
#include <thread>
#include <iostream>
#include "ostreamlock.h"
#include "random-generator.h"
#include "semaphore.h"
#include "thread-utils.h"
using namespace std;

static const unsigned int kLowPrepareTime = 10;
static const unsigned int kHighPrepareTime = 100;
static const unsigned int kLowProcessTime = 20;
static const unsigned int kHighProcessTime = 120;

static mutex rgenLock;
static RandomGenerator rgen;

static unsigned int getSleepDuration(unsigned int low, unsigned int high) {
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt(low, high);
}

static char prepareData() {
  sleep_for(getSleepDuration(kLowPrepareTime, kHighPrepareTime));
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt('A', 'Z');
}

static void processData(char ch) {
  sleep_for(getSleepDuration(kLowProcessTime, kHighProcessTime));
}

static const unsigned int kNumBuffers = 8;
static const unsigned int kNumCycles = 40;

/**
 * The character buffer represents the space in between two
 * networked computers.  The two semaphores are used to ensure
 * the reader or the writer block until there's data to consume
 * or space where data can be written.
 */

static char buffer[kNumBuffers];
static semaphore fullBuffers(0);
static semaphore emptyBuffers(1);

static void writer() {
  cout << oslock << "Writer: ready to write." << endl << osunlock;
  for (unsigned int i = 0; i < kNumCycles * kNumBuffers; i++) {
    char ch = prepareData();
    emptyBuffers.wait();   // don't try to write to a slot unless you know it's empty
    buffer[i % kNumBuffers] = ch;
    fullBuffers.signal();  // signal reader there's more stuff to read
    cout << oslock << "Writer: published data packet with character '" 
	 << ch << "'." << endl << osunlock;
  }
}

static void reader() {
  cout << oslock << "\t\tReader: ready to read." << endl << osunlock;
  for (unsigned int i = 0; i < kNumCycles * kNumBuffers; i++) {
    fullBuffers.wait();    // don't try to read from a slot unless you know it's full
    char ch = buffer[i % kNumBuffers];
    emptyBuffers.signal(); // signal writer there's a slot that can receive data
    processData(ch);
    cout << oslock << "\t\tReader: consumed data packet " 
	 << "with character '" << ch << "'." << endl << osunlock;
  }
}

int main(int argc, const char *argv[]) {
  thread w(writer);
  thread r(reader);
  w.join();
  r.join();
  return 0;
}
