/**
 * File: expensive-server-test.cc
 * ------------------------------
 * This is in place to levy many, many simulatanous requests
 * from our expensive server.
 */

#include <stdlib.h>       // for system
#include "thread-pool.h"

int main(int argc, char **argv) {
  ThreadPool requestPool(150);
  for (size_t i = 0; i < 1000; i++) {
    requestPool.schedule([] {
      system("wget --quiet --delete-after myth22.stanford.edu:33333");
    });
  }
  requestPool.wait();
  return 0;
}
