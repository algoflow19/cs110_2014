/**
 * File: tsh-parse.c
 * -----------------
 * Presents the implementation of parseLine, as documented
 * in tsh-parse.h.  Because parseLine needs to be able to
 * extract redirection files, there are lots and lots of
 * special cases.
 */

#include "tsh-parse.h"
#include "tsh-constants.h"
#include <string.h>
#include "exit-utils.h"

static const int kArgumentsArrayTooSmall = 1;

typedef enum {
  kNormal = 0,
  kInfile = 1 << 0,
  kOutfile = 1 << 1,
} parsestate_t;

static const char const *kWhitespace = " \t\n\r";
bool parseLine(char *command, char *arguments[], size_t maxArguments,
	       char **infile, char **outfile) {
  exitUnless(maxArguments > 1, kArgumentsArrayTooSmall,
	     stderr, "arguments array passed to parseline must be of size 2 or more.");
  *infile = *outfile = NULL;
  parsestate_t state = kNormal;
  int count = 0;
  while (true) {
    command += strspn(command, kWhitespace); // skips whitespace
    if (count == maxArguments || *command == '\0') break;
    bool quoted = *command == '\'';
    if (quoted) command++;
    char termch = quoted ? '\'' : ' ';
    char *end = strchr(command, termch);
    if (state == kNormal && !quoted && strchr("<>", *command) != NULL) {
      if (*command == '<' && *infile != NULL) {
        fprintf(stderr, "Multiple input redirections detected.\n");
        count = 0;
	break;
      } else if (*command == '>' && *outfile != NULL) {
	fprintf(stderr, "Multiple output redirections detected.\n");
	count = 0;
	break;
      }

      state = *command++ == '<' ? kInfile : kOutfile;
      continue;
    }
    
    if (state == kNormal) {
      arguments[count++] = command;
    } else if (state == kInfile) {
      if (strchr("<>", *command) != NULL) break;
      *infile = command;
    } else if (state == kOutfile) {
      if (strchr("<>", *command) != NULL) break;
      *outfile = command;
    }

    state = kNormal;
    if (end == NULL) {
      if (quoted) {
	fprintf(stderr, "Unmatched '.\n");
	count = 0;
      }
      break;
    }
    *end = '\0';
    command = end + 1;
  }

  if (count == maxArguments) {
    fprintf(stderr, "Too many tokens... ignoring command altogether.\n");
    count = 0;
  } else if (state != kNormal) {
    fprintf(stderr, "Must provide file for redirection.\n");
    count = 0;
  }
  
  arguments[count] = NULL;  
  if (count == 0) return false;
  bool bg = *arguments[count - 1] == '&';
  if (bg) arguments[--count] = NULL;
  return bg;
}
