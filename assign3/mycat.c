/**
 * mycat.c - Another handy routine for testing your tiny shell
 * 
 * usage: mycat
 * Reads from standard input and prints everything it 
 * reads to standard output.
 */

#include <stdio.h>
#include <stdlib.h>
#include "exit-utils.h"

static const int kWrongArgumentCount = 1;
static const int kKillError = 2;

int main(int argc, char *argv[]) {
  exitUnless(argc == 1, kWrongArgumentCount, 
	     stderr, "Usage: %s\n", argv[0]);
  char c;
  while ((c = getchar()) != EOF) 
    putchar(c);
  return 0;
}
