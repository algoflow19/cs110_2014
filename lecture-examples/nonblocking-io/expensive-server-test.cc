/**
 * File: expensive-server-test.cc
 * ------------------------------
 * This is in place to levy many, many simulatanous requests
 * from our expensive server.
 */

#include <cstdlib>       // for system
#include "thread-pool.h"
using namespace std;

int main(int argc, char **argv) {
  ThreadPool requestPool(64);
  for (size_t i = 0; i < 10000; i++) {
    requestPool.schedule([] {
      system("wget --quiet --delete-after myth22.stanford.edu:12345");
    });
  }
  requestPool.wait();
  return 0;
}
