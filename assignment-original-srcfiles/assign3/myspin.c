/**
 * myspin.c - A handy program for testing your tiny shell 
 * 
 * usage: myspin <n>
 * Sleeps for <n> seconds in 1-second chunks.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "exit-utils.h"

static const int kWrongArgumentCount = 1;

int main(int argc, char *argv[]) {
  exitUnless(argc == 2, kWrongArgumentCount,
	     stderr, "Usage: %s <n>\n", argv[0]);
  size_t secs = atoi(argv[1]);
  for (size_t i = 0; i < secs; i++) {
    sleep(1);
  }
  
  return 0;
}
