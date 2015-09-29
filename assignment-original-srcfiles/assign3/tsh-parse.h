/**
 * File: tsh-parse.h
 * -----------------
 * Exports a function that knows how to parse an
 * arbitrary string into tokens and store them into 
 * an argument vector, all while extracting the names of
 * input and/or output redirection files.
 */

#ifndef _tsh_parse_
#define _tsh_parse_

#include <stdbool.h>
#include <stddef.h>

/**
 * Accepts a command string, replaces all leading spaces after tokens
 * with '\0' characters, and updates the arguments array so that each
 * slot (up to a NULL terminating pointer) addresses the leading characters
 * of the in-place tokenized command line.  Substrings within single quotes
 * are taken to be single tokens (without the single quotes).
 * 
 * If, for instance, commandLine contains "ls -lta\0", it would be
 * updated in place to look like "ls\0-lta\0", arguments[0] would store
 * the address of the leading 'l', arguments[1] would store the address of
 * the '-', and arguments[2] would contain NULL.
 *
 * If commandLine contains "   gcc    -o 'my awesome app'  a.c b.c   c.c\0    ",
 * it would be updated to contain "   gcc\0   -o\0'my awesome app'\0  a.c\0b.c\0  c.c\0    ",
 * and arguments[0] would address the leading 'g', arguments[1] would 
 * address the '-',  arguments[2] would contain the address of the 'm', etc. 
 * (and arguments[6] would be updated to contain NULL.
 *
 * This version of parseLine also lifts out the names of input and
 * outfile files to be used for redirections.  It's interesting to
 * note that the "<" and ">" can appear in any order, and that they 
 * may even precede all other tokens.  In particular, these are all 
 * legitimate command lines:
 *
 *    tsh> cat < input.txt > output.txt
 *    tsh> cat > output.txt < input.txt
 *    tsh> > output.txt < input.txt cat
 *    tsh> < input.txt > output.txt cat
 *    tsh> < input.txt cat > output.txt
 *    tsh> > output.txt cat < input.txt
 *
 * If there are no input and/or output files specified via redirection,
 * the the spaces addressed by the corresponding char **s are populated
 * with NULLs.
 *
 * parseLine returns true if the user has requested a kBackground job, and 
 * false if the user has requested a kForeground job.  (Of course, the command
 * line expresses a background job whenever it ends in the token "&".)
 */

bool parseLine(char *commandLine, char *arguments[], size_t maxArguments,
	       char **infile, char **outfile);

#endif // _tsh_parse_
