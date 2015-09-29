/**
 * File: tsh-jobs.h
 * ----------------
 * Defines the struct job type and a collection 
 * of utility functions that can be used to 
 * access and manipulate them.
 */

#ifndef _tsh_jobs_
#define _tsh_jobs_

#include "tsh-constants.h"
#include <stdbool.h>
#include <sys/types.h>

/**
 * Defines the four different states a job can be in.
 */

enum {
  kUndefined = 0, kBackground, kForeground, kStopped,
};

/**
 * Type: struct job_t
 * ------------------
 * Manages all of the information about a currently executing job.
 *
 *   pid: the job's process id
 *   jid: the job id [1, 2, 3, 4, etc.]
 *   state: kUndefined, kBackground, kForeground, or kStopped 
 *   commandLine: The original command line used to create the job
 */

typedef struct job_t {
  pid_t pid;
  int jid;
  int state;
  char commandLine[kMaxLine];
} job_t;

/**
 * Function: clearJob
 * ------------------
 * Clears out the entries in the supplied job_t.
 */
void clearJob(job_t *job);

/**
 * Function: initJobs
 * ------------------
 * Clears all of the jobs in the provided job_t array.
 */
void initJobs(job_t jobs[]);

/**
 * Function: getMaxJobID
 * ---------------------
 * Returns the largest job ID associated with any 
 * currently executing job.
 */
int getMaxJobID(job_t jobs[]); 

/**
 * Function: addJob
 * ----------------
 * Adds a new job to the provided job list, using the
 * provided pid, state, and commandLine to configure the
 * new job entry.
 *
 * Returns true if the job was successfully added to the
 * jobs list, and returns false otherwise (because the job
 * list was full.)
 */
bool addJob(job_t jobs[], pid_t pid, int state, const char *commandLine);

/**
 * Function: deleteJob
 * -------------------
 * Removes the job with the specified pid from the
 * specified jobs list.
 *
 * Returns true if the job with the provided pid was in the
 * list and was removed, and false if the pid was invalid.
 */
bool deleteJob(job_t jobs[], pid_t pid); 

/**
 * Function: getFGJobPID
 * ---------------------
 * Returns the pid of the current foreground job, or
 * 0 if there is no such job.
 */
pid_t getFGJobPID(job_t jobs[]);

/**
 * Function: getJobByPID
 * ---------------------
 * Find and return the address of the job associated
 * with the specified pid, or return NULL if there is
 * no such job because the pid is invalid.
 */
job_t *getJobByPID(job_t jobs[], pid_t pid);

/**
 * Function: getJobByJID
 * ---------------------
 * Find and return the address of the job associated
 * with the specified jid, or return NULL if the supplied
 * jid is invalid.
 */
job_t *getJobByJID(job_t jobs[], int jid); 

/**
 * Function: getJIDFromPID
 * -----------------------
 * Return the jid associated with the specified pid (or
 * return 0 if the provided pid is invalid.
 */
int getJIDFromPID(pid_t pid); 

/**
 * Function: listJobs
 * ------------------
 * Publishes information about all currently executing jobs.
 */
void listJobs(job_t jobs[]);

#endif
