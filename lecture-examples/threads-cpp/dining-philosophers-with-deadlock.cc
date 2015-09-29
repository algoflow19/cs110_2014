/**
 * File: dining-philosophers.cc
 * ----------------------------
 * This program implements the classic dining philosophers
 * simulation.
 */

#include <thread>
#include <mutex>
#include <iostream>
#include "ostreamlock.h"
#include "random-generator.h"
#include "thread-utils.h"
using namespace std;

/**
 * Defines a collection of timing constants used for 
 * the dining philosopher simulation.
 */

static const unsigned int kLowThinkTime = 100;
static const unsigned int kHighThinkTime = 2000;
static const unsigned int kLowSleepTime = 25;
static const unsigned int kHighSleepTime = 50;

/**
 * Defines the single RandomGenerator class used to generate
 * random timing amounts to allow for variety in the dining 
 * philosopher simulation.
 */

static mutex rgenLock;
static RandomGenerator rgen;

static unsigned int getThinkTime() {
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt(kLowThinkTime, kHighThinkTime);
}

static unsigned int getEatTime() {
  lock_guard<mutex> lg(rgenLock);
  return rgen.getNextInt(kLowSleepTime, kHighSleepTime);
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
 * is an atomic resource that's either held or not.
 */

static mutex forks[kNumForks];
static void think(unsigned int id) {
  cout << oslock << id << " starts thinking." << endl << osunlock;
  sleep_for(getThinkTime());
  cout << oslock << id << " all done thinking. " << endl << osunlock;
}

static void eat(unsigned int id) {
  unsigned int left = id;
  unsigned int right = (id + 1) % kNumForks;
  forks[left].lock();
  sleep_for(5000);
  forks[right].lock();
  cout << oslock << id << " starts eating om nom nom nom." << endl << osunlock;
  sleep_for(getEatTime());
  cout << oslock << id << " all done eating." << endl << osunlock;
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
  for (unsigned int i = 0; i < kNumPhilosophers; i++) philosophers[i] = thread(philosopher, i);
  for (thread& p: philosophers) p.join();
  return 0;
}
