/**
 * File: tsh-parse-test.c
 * ----------------------
 * Provides a test framework to exercise that
 * parseLine function exported by tsh-parse.[hc].
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tsh-parse.h"

static const char *const kCommandPrompt = "parse-test>";
static const unsigned short kMaxCommandLength = 1024; // in characters
static const unsigned short kMaxArgumentCount = 128;

static void pullrest() {
  while (getchar() != '\n');
}

static void readcommand(char command[], size_t len) {
  char control[64] = {'\0'};
  command[0] = '\0';
  sprintf(control, "%%%zu[^\n]%%c", len);
  while (true) {
    printf("%s ", kCommandPrompt);
    char termch;
    if (scanf(control, command, &termch) < 2) { pullrest(); return; }
    if (termch == '\n') return;
    fprintf(stderr, "Command shouldn't exceed %hu characters.  Ignoring.\n", kMaxCommandLength);
    pullrest();
  }
}

int main(int argc, char *argv[]) {
  while (true) {
    char command[kMaxCommandLength + 1];
    readcommand(command, sizeof(command) - 1);
    if (feof(stdin)) break;
    char *arguments[kMaxArgumentCount], *infile = NULL, *outfile = NULL;
    bool bg = parseLine(command, arguments, kMaxArgumentCount, 
			&infile, &outfile);
    if (arguments[0] == NULL) continue;
    if (strcasecmp(arguments[0], "quit") == 0) exit(0);
    printf("Arguments:\n");
    size_t i = 0;
    for (; arguments[i] != NULL; i++) {
      printf(" arguments[%zu] = \"%s\"\n", i, arguments[i]);
    }
    printf(" arguments[%zu] = %s\n", i, arguments[i] == NULL ? "NULL" : "non-NULL");
    printf("Background? %s\n", bg ? "yes" : "no");
    if (infile != NULL) printf("Input file: \"%s\"\n", infile);
    if (outfile != NULL) printf("Outfile file: \"%s\"\n", outfile);
  }
  
  printf("\n");
  return 0;
}
