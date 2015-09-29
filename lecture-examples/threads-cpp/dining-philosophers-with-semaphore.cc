/**
 * File: dining-philosophers.cc
 * ----------------------------
 * This program implements the classic dining philosophers
 * simulation using our custom semaphore class, which is
 * declared in /usr/class/cs110/local/include/semaphore.h.
 * You can inspect the implementation by looking in
 * /usr/class/cs110/local/src/threads/semaphore.cc
 */

#include <thread>
#include <mutex>
#include <iostream>
#include "semaphore.h"
#include "ostreamlock.h"
#include "thread-utils.h"
#include "random-generator.h"
using namespace std;

/**
 * Defines a collection of timing constants used for 
 * the dining philosopher simulation.
 */

static const unsigned int kMinThinkTime = 100;
static const unsigned int kMaxThinkTime = 2000;
static const unsigned int kMinEatTime = 25;
static const unsigned int kMaxEatTime = 50;

/**
 * Defines the single RandomGenerator class used to generate
 * random timing amounts to allow for variety in the dining 
 * philosopher simulation.
 */

static mutex rgenLock;
static RandomGenerator rgen;

static unsigned int getThinkDuration() {
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt(kMinThinkTime, kMaxThinkTime);
}

static unsigned int getEatDuration() {
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt(kMinEatTime, kMaxEatTime);
}

/**
 * Defines the collection of constants (not related at all to timing)
 * needed for the dining philosophers simulation.
 */

static const unsigned int kNumPhilosophers = 5;
static const unsigned int kNumForks = kNumPhilosophers;
static const unsigned int kNumMeals = 3;

/**
 * Global variables representing shared resources.
 * Note that each fork is modeled as a mutex, because each fork
 * is an atomic resource that's either held or not.  The numAllowed
 * global is used to prevent deadlock, and in this version we model
 * numAllowed as a generalized counter with atomic wait and signal
 * methods.  Understand that semaphore::wait blocks if the encapsulated
 * count is 0, and it blocks until some other thread signals it.
 */

static mutex forks[kNumForks];
static semaphore numAllowed(kNumForks - 1);

static void think(unsigned int id) {
  cout << oslock << id << " starts thinking." << endl << osunlock;
  sleep_for(getThinkDuration());
  cout << oslock << id << " all done thinking. " << endl << osunlock;
}

static void eat(unsigned int id) {
  unsigned int left = id;
  unsigned int right = (id + 1) % kNumForks;
  numAllowed.wait(); // atomic --, that blocks on 0
  forks[left].lock();
  forks[right].lock();
  cout << oslock << id << " starts eating om nom nom nom." << endl << osunlock;
  sleep_for(getEatDuration());
  cout << oslock << id << " all done eating." << endl << osunlock;
  numAllowed.signal(); // atomic ++, never blocks, possibly unblocks other waiting threads
  forks[left].unlock();
  forks[right].unlock();
}

static void philosopher(unsigned int id) {
  for (unsigned int i = 0; i < kNumMeals; i++) {
    think(id);
    eat(id);
  }
}

int main(int argc, const char *argv[]) {
  thread philosophers[kNumPhilosophers];
  for (unsigned int i = 0; i < kNumPhilosophers; i++) {
    philosophers[i] = thread(philosopher, i);
  }
  
  for (unsigned int i = 0; i < kNumPhilosophers; i++) {
    philosophers[i].join();
  }
  
  return 0;
}
