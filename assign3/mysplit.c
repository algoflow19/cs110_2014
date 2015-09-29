/** 
 * File: mysplit.c
 * ---------------
 * Short routine for testing simplesh.  All is does is
 * fork a child that spins for <n> seconds in 1-second bursts.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "exit-utils.h"

int main(int argc, char *argv[]) {
  exitUnless(argc == 2, 0, stderr, "Usage: %s <n>\n", argv[0]);
  size_t secs = atoi(argv[1]);
  pid_t pid = fork();
  exitIf(pid == -1, 0, stderr, "fork function failed.\n");
  if (pid == 0) {
    for (size_t i = 0; i < secs; i++) {
      sleep(1);
    }
  } else {
    wait(NULL);
  }
  return 0;
}
