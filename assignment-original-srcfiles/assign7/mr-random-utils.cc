/**
 * File: mr-random-utils.h
 * -----------------------
 * Presents the implementation of those functions exported by
 * the mr-random-utils.h module
 */

#include "mr-random-utils.h"
#include <cstdlib>            // for srand, random
#include <ctime>              // for time
#include <unistd.h>           // for sleep
#include "random-generator.h" // for RandomGenerator class
using namespace std;

static RandomGenerator rgen;
void sleepRandomAmount(size_t low, size_t high) {
  int sleepAmount = rgen.getNextInt(low, high);
  sleep(sleepAmount);
}

bool randomChance(double probability) {
  return rgen.getNextBool(probability);
}
