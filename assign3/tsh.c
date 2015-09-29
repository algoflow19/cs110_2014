/**
 * File: tsh.c
 * -----------
 * Your name: <your name here>
 * Your favorite shell command: <your favorite shell command here> 
 */

/* Header files */
#include <stdbool.h>       // for bool type
#include <stdio.h>         // for printf, etc
#include <stdlib.h>        
#include <unistd.h>        // for fork
#include <string.h>        // for strlen, strcasecmp, etc
#include <ctype.h>
#include <signal.h>        // for signal
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>         // for open
#include <sys/wait.h>      // for wait, waitpid
#include <errno.h>
#include "tsh-state.h"
#include "tsh-constants.h"
#include "tsh-parse.h"
#include "tsh-jobs.h"
#include "tsh-signal.h"
#include "exit-utils.h"    // provides exitIf, exitUnless

#define QUIT 1
#define FGBG 2
#define JOBS 3
#define NO_ARG_ERR -1
#define INVALID_ARG_ERR -2

/* Error returns */
static const int kForkFailed = 1;
static const int kWaitFailed = 2;
static const int kExecFailed = 3;
static const int kReadFailed = 4;
static const int kWriteFailed = 5;

/* Redirected fds to forward input/output of builtin commands */
/* They are global, so that they can be relinquished from a signal handler */
static int redirectedStdIn = -1;
static int redirectedStdOut = -1;

/* Helper functions */
static int blockSigChild();
static int unblockSigChild();
static int validateBackgroundForegroundCommand(char *argv[], int *jid, int *pid);
static int handleRedirectionForBuiltIn(char *infile, char *outfile);
static int ishandleBuiltin(char *argv[]);
static void closeRedirectedFdsIfAny();
static void handleRedirectionForCommand(char *infile, char *outfile);
static pid_t forkJob();
static void resetToDefaultSignalHandlers();

/**
 * Function : blockSigChild
 * -------------------------------
 *  Blocks sigchild signal on the process from which it
 *  gets called. 
 *
 *  Note : Previous signmask gets overwritten, use with
 *  caution
 */
static int blockSigChild() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    return sigprocmask(SIG_BLOCK, &mask, NULL);
}

/**
 * Function : unblockSigChild
 * --------------------------------
 *  Unblocks sigchild signal on the process from it gets
 *  called
 *
 *  Note : Previous sigmask gets overwritten, use
 *  with caution
 */
static int unblockSigChild() {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    return sigprocmask(SIG_UNBLOCK, &mask, NULL);
}

/** 
 * Function : waitfg
 * ---------------------
 * Block until process pid is no longer the foreground process
 */
static void waitfg(pid_t pid) {
    sigset_t mask;
    sigemptyset(&mask);
    while (true) {
        /* Wake up if we have received a signal */
        sigsuspend(&mask);
        /* Check if the foreground process is done */
        if (pid != getFGJobPID(jobs))
            break;
    }
}

/**
 * Function : handleRedirectionForBuiltIn
 * ----------------------------------------------
 *  This function opens fds for infile and outfile specified as part of 
 *  builtin command 
 */
static int handleRedirectionForBuiltIn(char *infile, char *outfile) {
    int fd;
    if ((infile)) {
        /* Try to open the infile with read only permission */
        if ((fd = open(infile, O_RDONLY)) == -1) {
            dprintf(STDERR_FILENO, "No such file or directory: %s\n", infile);
            redirectedStdIn = -1;
            return -1;
        }
        redirectedStdIn = fd;
    } else {
        redirectedStdIn = -1;
    }
    if ((outfile)) {
        /* 
         * If file does not exist attempt to open the file with read/write permissions 
         * for the current user
         */
        if ((fd = open(outfile, O_WRONLY | O_CREAT, (S_IRUSR | S_IWUSR))) == -1) {
            /* 
             * May be the current user just has just write permissions on the directory
             * mentioned in outfile pathname. Trying one more time before giving up.
             */
            if ((fd = open(outfile, O_WRONLY | O_CREAT, S_IWUSR)) == -1) {
                dprintf(STDERR_FILENO, "Error opening file: %s\n", outfile);
                redirectedStdOut = -1;
                return -1;
            }
        }
        redirectedStdOut = fd;
    } else {
        redirectedStdOut = -1;
    }
    return 1;
}

/**
 * Function : validateBackgroundForegroundCommand
 * -------------------------------------------------------
 *  This function validates the input for background and foreground command.
 *  Fills jobid and pid of the caller function when their addresses are passed
 *  as arguments.
 */
static int validateBackgroundForegroundCommand(char *argv[], int *jid, int *pid) {
    char first = '\0';
    /* Return error if there is no second parameter */
    if (!(argv[1]))
        return NO_ARG_ERR;
    /* Return error if the second parameter is a NULL string */
    int len = strlen(argv[1]);
    if (len == 0) 
        return NO_ARG_ERR;

    /* Validate second parameter */
    if (1 == sscanf(argv[1],"%c", &first)) {
        /* If the first character is a %, it is a job id */
        if (first == '%') {
            if (len > 1) {
                char *next = ++argv[1];
                /* Check if it is a digit */
                if (isdigit(*next)) {
                    *jid = atoi(next);
                    *pid = 0;
                    /* 
                     * Intentionally avoiding the validation of argv[2] and above 
                     * to comply with reference solution.
                     */
                    return 1;
                }
            }
            return INVALID_ARG_ERR;
        } else {
            /* It must be a pid, therefore should be a digit */
            if (isdigit(first)) {
                *pid = atoi(argv[1]);
                *jid = 0;
                /* 
                 * Intentionally avoiding the validation of argv[2] and above 
                 * to comply with reference solution.
                 */
                return 1;
            }
            return INVALID_ARG_ERR;
        }
    }
    return INVALID_ARG_ERR;
}
/**
 * Function : handleBackgroundForegroundBuiltin
 * --------------------------------------------------
 * Execute the builtin bg and fg commands
 */
static void handleBackgroundForegroundBuiltin(char *argv[], int outfd) {
    int jid;
    int pid;
    job_t *entry;

    /* Validating foreground / background commands */
    int err = validateBackgroundForegroundCommand(argv, &jid, &pid);
    if (err < 0) {
        switch (err) {
            case NO_ARG_ERR :
                dprintf(outfd, "%s command requires PID or %%jobid argument\n", argv[0]);
                break;
            case INVALID_ARG_ERR :
                dprintf(outfd, "%s: argument must be a PID or %%jobid\n", argv[0]);
                break;
        }
        /* Unblock child before returning */
        unblockSigChild();
        return;
    }

    /*
     * fg, bg commands can have either jobid or pid, never together 
     */
    if (pid) {
        jid = getJIDFromPID(pid);
        if (!(jid)) {
            dprintf(outfd, "(%d): No such process\n", pid);
            unblockSigChild();
            return;
        }
        entry = getJobByJID(jobs, jid);
    } else {
        entry = getJobByJID(jobs, jid);
        if (!(entry)) {
            dprintf(outfd, "%%%d: No such job\n", jid);
            unblockSigChild();
            return;
        }
        pid = entry->pid;
    }

    if (strcasecmp(argv[0], "fg") == 0) {
        /* Sending SIGCONT */
        if (kill(-pid, SIGCONT) == -1)
            dprintf(outfd, "Kill to group id %d failed\n", pid);
        entry->state = kForeground;
        /* Unblock sig child before waiting for foreground process to exit */
        unblockSigChild();
        waitfg(pid);
    } else {
        /* Sending SIGCONT */
        if (kill(-pid, SIGCONT) == -1)
            dprintf(outfd, "Kill to group id %d failed\n", pid);
        entry->state = kBackground;
        /* Printing background job before unblocking sigchld */
        dprintf(outfd, "[%d] (%d) %s\n", jid, pid, entry->commandLine);
        unblockSigChild();
    }
}

/**
 * Function : ishandleBuiltin
 * ----------------------------------------
 *  This function checks if the input is a builtin command
 */
static int ishandleBuiltin(char *argv[]) {
    if (strcasecmp(argv[0], "quit") == 0)
        return QUIT;
    if ((strcasecmp(argv[0], "fg") == 0) || (strcasecmp(argv[0], "bg") == 0))
        return FGBG;
    if (strcasecmp(argv[0], "jobs") == 0)
        return JOBS;
    return -1;
}

/**
 * Function : closeRedirectedFdsIfAny
 * ------------------------------------------
 *  This function closes any of the redirected fds opened for builtin commands
 */
static void closeRedirectedFdsIfAny() {
    if (redirectedStdIn >= 0) {
        close(redirectedStdIn);
        redirectedStdIn = -1;
    }
    if (redirectedStdOut >= 0) {
        close(redirectedStdOut);
        redirectedStdIn = -1;
    }
}

/**
 * Function : handleBuiltin
 * -----------------------------------
 * If the user has typed a built-in command then execute 
 * it immediately.  Return true if and only if the command 
 * was a builtin and executed inline.
 */
static bool handleBuiltin(char *argv[], char *infile, char *outfile) {
    int builtin;

    /* Check if the command is a builtin */
    if ((builtin = ishandleBuiltin(argv)) < 0)
        return false;

    /* Handle redirection if necessary */
    if (handleRedirectionForBuiltIn(infile, outfile) < 0) {
        closeRedirectedFdsIfAny();
        return true;
    }

    switch (builtin) {
        case QUIT:
            /* Releasing all resources */
            closeRedirectedFdsIfAny();
            killAllJobs(jobs);
            exit(0);
            break;
        case FGBG:
            /* Handle fg/bg commands */
            handleBackgroundForegroundBuiltin(argv, (redirectedStdOut > 0 ? redirectedStdOut : STDOUT_FILENO));
            break;
        case JOBS:
            /* List Jobs */
            listJobsToFd(jobs, (redirectedStdOut > 0 ? redirectedStdOut : STDOUT_FILENO));
            unblockSigChild();
            break;
    }

    /* Closing opened fds */
    closeRedirectedFdsIfAny();
    return true;
}

/**
 * Function : handleSIGCHLD
 * ----------------------------------
 * The kernel sends a SIGCHLD to the shell whenever a child job terminates 
 * (becomes a zombie), or stops because it receives a SIGSTOP or SIGTSTP signal.  
 * The handler reaps all available zombie children, but doesn't wait for any other
 * currently running children to terminate.  
 */
 static void handleSIGCHLD(int unused) {
    pid_t pid;
    int status;
    while (true) {
        pid = waitpid(-1, &status, (WUNTRACED | WNOHANG));
        if (pid <= 0) break;
        /* Handling child receiving a stop signal */
        if (WIFSTOPPED(status)) {
            printf("Job [%d] (%d) stopped by signal %d\n", getJIDFromPID(pid), pid, WSTOPSIG(status));
            job_t * entry = getJobByPID(jobs, pid);
            entry->state = kStopped;
        } else {
            /* Handling child receiving a termination signal */
            if (WIFSIGNALED(status))
                printf("Job [%d] (%d) terminated by signal %d\n", getJIDFromPID(pid), pid, WTERMSIG(status));
            /* Fall through, delete job when child exits */
            deleteJob(jobs, pid);
        }
    }
    exitUnless(pid == 0 || errno == ECHILD, kWaitFailed,
            stderr, "waitpid failed within handleSIGCHLD.\n");
}

/**
 * Function : handleSIGTSTP
 * --------------------------------
 * The kernel sends a SIGTSTP to the shell whenever
 * the user types ctrl-z at the keyboard.  Catch it and suspend the
 * foreground job by sending it a SIGTSTP.
 */
static void handleSIGTSTP(int sig) {
    pid_t pid = getFGJobPID(jobs);
    if (pid) {
        if (kill(-pid, sig) == -1)
            dprintf(STDERR_FILENO, "Kill to group id %d failed\n", pid);
    }
}

/**
 * Function : handleSIGINT
 * -----------------------------------
 * The kernel sends a SIGINT to the shell whenver the
 * user types ctrl-c at the keyboard.  Catch it and send it along
 * to the foreground job.  
 */
static void handleSIGINT(int sig) {
    pid_t pid = getFGJobPID(jobs);
    if (pid) {
        if (kill(-pid, sig) == -1)
            dprintf(STDERR_FILENO, "Kill to group id %d failed\n", pid);
    }
}

/**
 * Function : handleSIGQUIT
 * -------------------------------------
 * The driver program can gracefully terminate the
 * child shell by sending it a SIGQUIT signal.
 */
static void handleSIGQUIT(int sig) {
  printf("Terminating after receipt of SIGQUIT signal\n");
  /* Free all resources before exiting */
  closeRedirectedFdsIfAny();
  killAllJobs(jobs);
  exit(1);
}

/**
 * Function : usage
 * ----------------------
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
 * Function : forkJob
 * ------------------------------------
 *  Forks a child and handles any anomaly by exiting with proper
 *  error message
 */
static pid_t forkJob() {
    pid_t pid = fork();
    exitIf(pid == -1, kForkFailed, stderr, "fork function failed.\n");
    return pid;
}

/**
 * Function : handleRedirectionForCommand
 * ----------------------------------------
 *  This function opens fds for infile and outfile if present and dups
 *  the stdin and stdout of the process. It closes the fd of the infile/
 *  outfile after dup, so that the refcnt of the entry in the open file 
 *  table becomes 1 and thereby gets deleted when the process exits, closing
 *  stdin and stdout.
 */
static void handleRedirectionForCommand(char *infile, char *outfile) {
    int fd;
    if ((infile)) {
        exitIf((fd = open(infile, O_RDONLY)) == -1, kReadFailed, stderr, "No such file or directory: %s\n", infile);
        /* Duping fd with stdin */
        dup2(fd, STDIN_FILENO);
        /* Closing fd to get back refcnt to 1 */
        close(fd);
    }
    if ((outfile)) {
        if ((fd = open(outfile, O_WRONLY | O_CREAT, (S_IRUSR | S_IWUSR))) == -1) {
            /*
             * May be the current user just has just write permissions on the directory
             * mentioned in outfile pathname. Trying one more time before giving up.
             */
            exitIf((fd = open(outfile, O_WRONLY | O_CREAT, S_IWUSR)) == -1, kWriteFailed, stderr, "Error opening file: %s\n", outfile);
        }
        /* Duping fd with stdout */
        dup2(fd, STDOUT_FILENO);
        /* Closing fd to get back refcnt to 1 */
        close(fd);
    }
}

/**
 * Function : resetToDefaultSignalHandlers
 * -------------------------------------------------
 *  Resets the signal handler for SIGQUIT, SIGINT, SIGTSTP and SIGCHLD
 */
static void resetToDefaultSignalHandlers() {
    signal(SIGQUIT, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

/**
 * Function : eval
 * ----------------------------
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately.  Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
 */
static void eval(char commandLine[]) {
    char *infile;
    char *outfile;
    char *arguments[kMaxArgs];
    char backupCommandLine[kMaxLine];
    /* Backing up before parseLine scrambles commandLine */
    strcpy(backupCommandLine, commandLine);
    bool background = parseLine(commandLine, arguments, kMaxArgs, &infile, &outfile); 
    // Empty command - Somebody pressed enter or just spaces followed by enter
    if (!(arguments[0]))
        return;
    // Entering critical region, block sigchild
    blockSigChild();
    if(!handleBuiltin(arguments, infile, outfile)) {
        if (canNewJobBeAdded(jobs) == false) {
            printf("Tried to create too many jobs.\n");
            return;
        }
        pid_t pid = forkJob();  
        if (pid == 0) {
            // Child
            setpgid(0, 0);
            /* 
             * We need to reset procmask and signal handlers so that the 
             * child does not corrupt the
             * joblist through the custom signal handler it inherits from 
             * its parent under exceptional conditions when execvp fails.
             */

            /* Unblocks sigchild and resets Signal mask to receive any signal */
            unblockSigChild();
            /* Resets signal handlers that were modified in the parent */
            resetToDefaultSignalHandlers();
            /* Redirects the stdin and stdout if necessary */
            handleRedirectionForCommand(infile, outfile);
            exitIf(execvp(arguments[0], arguments) == -1,
                    kExecFailed, stderr, "%s: Command not found\n", arguments[0]);
        }
        // Parent
        int state = background ? kBackground : kForeground;
        // No need to check for return value as we had preemptively 
        // checked if the job could be added
        addJob(jobs, pid, state, backupCommandLine);
        if (!background) {
            // Foreground
            unblockSigChild();
            waitfg(pid);
        }
        else {
            // Background
            printf("[%d] (%d) %s\n", getJIDFromPID(pid), pid, backupCommandLine);
            unblockSigChild();
        }
    }
}

/**
 * Function : mergeFileDescriptors
 * --------------------------------------------
 * Redirect stderr to stdout (so that driver will get all output
 * on the pipe connected to stdout) 
 */
static void mergeFileDescriptors() {
  dup2(STDOUT_FILENO, STDERR_FILENO);
}

/**
 * Function : main
 * --------------------------
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

  /* Infinite loop */
  while (true) {
    if (showPrompt) {
      printf("%s", kPrompt);
      fflush(stdout);
    }

    char command[kMaxLine];
    fgets(command, kMaxLine, stdin);
    if (feof(stdin)) break;
    command[strlen(command) - 1] = '\0'; // overwrite fgets's \n
    eval(command);
    fflush(stdout);
  }
 
  /* Unlikely to ever reach here */
  fflush(stdout);  
  closeRedirectedFdsIfAny();
  return 0;
}

