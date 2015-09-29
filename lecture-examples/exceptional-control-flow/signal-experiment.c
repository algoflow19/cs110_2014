#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

void handleSIGSEGV(int sig) {
  printf("Uh oh! You segfaulted!\n");
  exit(0);
}

int main(int argc, char *argv[]) {
  signal(SIGSEGV, handleSIGSEGV);
  *(int *)NULL = 0;
  printf("I never get here.\n");
  return 0;
}
