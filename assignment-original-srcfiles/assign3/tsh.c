/**
 * File: tsh.c
 * -----------
 * Your name: <your name here>
 * Your favorite shell command: <your favorite shell command here> 
 */

#include <stdbool.h>       // for bool type
#include <stdio.h>         // for printf, etc
#include <stdlib.h>        
#include <unistd.h>        // for fork
#include <string.h>        // for strlen, strcasecmp, etc
#include <ctype.h>
#include <signal.h>        // for signal
#include <sys/types.h>
#include <sys/wait.h>      // for wait, waitpid
#include <errno.h>

#include "tsh-state.h"
#include "tsh-constants.h"
#include "tsh-parse.h"
#include "tsh-jobs.h"
#include "tsh-signal.h"
#include "exit-utils.h"    // provides exitIf, exitUnless

/** 
 * Block until process pid is no longer the foreground process
 */
/* static */ void waitfg(pid_t pid) {
  // provide your own implementation here
  // static keyword is commented out to suppress CT warning
}

/**
 * Execute the builtin bg and fg commands
 */
/* static */ void handleBackgroundForegroundBuiltin(char *argv[]) {
  // provide your own implementation here
  // static keyword is commented out to suppress CT warning
}

/**
 * If the user has typed a built-in command then execute 
 * it immediately.  Return true if and only if the command 
 * was a builtin and executed inline.
 */
/* static */ bool handleBuiltin(char *argv[]) {
  // provide your own implementation here
  // static keyword is commented out to suppress CT warning
  return false;
}

/**
 * The kernel sends a SIGCHLD to the shell whenever a child job terminates 
 * (becomes a zombie), or stops because it receives a SIGSTOP or SIGTSTP signal.  
 * The handler reaps all available zombie children, but doesn't wait for any other
 * currently running children to terminate.  
 */
/* static */ void handleSIGCHLD(int unused) {
  // provide your own implementation here
}

/**
 * The kernel sends a SIGTSTP to the shell whenever
 * the user types ctrl-z at the keyboard.  Catch it and suspend the
 * foreground job by sending it a SIGTSTP.
 */
static void handleSIGTSTP(int sig) {
  // provide your own implementation here
}

/**
 * The kernel sends a SIGINT to the shell whenver the
 * user types ctrl-c at the keyboard.  Catch it and send it along
 * to the foreground job.  
 */
static void handleSIGINT(int sig) {
  // provide your own implementation here
}

/**
 * The driver program can gracefully terminate the
 * child shell by sending it a SIGQUIT signal.
 */
static void handleSIGQUIT(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  exit(1);
}

/**
 * Prints a nice little usage message to make it clear how the
 * user should be invoking tsh.
 */
static void usage() {
  printf("Usage: ./tsh [-hvp]\n");
  printf("   -h   print this message\n");
  printf("   -v   print additional diagnostic information\n");
  printf("   -p   do not emit a command prompt\n");
  exit(1);
}

/**
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately.  Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
static void eval(char commandLine[]) {
  // provide your own implementation here
}

/**
 * Redirect stderr to stdout (so that driver will get all output
 * on the pipe connected to stdout) 
 */
static void mergeFileDescriptors() {
  dup2(STDOUT_FILENO, STDERR_FILENO);
}

/**
 * Defines the main read-eval-print loop
 * for your simplesh.
 */
int main(int argc, char *argv[]) {
  mergeFileDescriptors();
  while (true) {
    int option = getopt(argc, argv, "hvp");
    if (option == EOF) break;
    switch (option) {
    case 'h':
      usage();
      break;
    case 'v': // emit additional diagnostic info
      verbose = true;
      break;
    case 'p':           
      showPrompt = false;
      break;
    default:
      usage();
    }
  }
  
  installSignalHandler(SIGQUIT, handleSIGQUIT); 
  installSignalHandler(SIGINT,  handleSIGINT);   // ctrl-c
  installSignalHandler(SIGTSTP, handleSIGTSTP);  // ctrl-z
  installSignalHandler(SIGCHLD, handleSIGCHLD);  // terminated or stopped child
  initJobs(jobs);  

  while (true) {
    if (showPrompt) {
      printf("%s", kPrompt);
      fflush(stdout);
    }

    char command[kMaxLine];
    fgets(command, kMaxLine, stdin);
    if (feof(stdin)) break;
    command[strlen(command) - 1] = '\0'; // overwrite fgets's \n
    if (strcasecmp(command, "quit") == 0) break;
    eval(command);
    fflush(stdout);
  }
  
  fflush(stdout);  
  return 0;
}
