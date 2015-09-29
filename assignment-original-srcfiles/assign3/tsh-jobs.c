/**
 * File: tsh-jobs.c
 * ---------------- 
 * Presents the implementation of all of the 
 * jobs manipulation functions defined and 
 * documented in tsh-jobs.h
 */

#include "tsh-state.h"
#include "tsh-jobs.h"
#include <stdio.h>    // for printf
#include <string.h>   // for strcpy

// standalone module state
static int nextJobID = 1;

void listJobs(job_t jobs[]) {
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid != 0) {
      printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
      switch (jobs[i].state) {
      case kBackground:
	printf("Running ");
	break;
      case kForeground: 
	printf("Foreground ");
	break;
      case kStopped:
	printf("Stopped ");
	break;
      default:
	printf("listjobs: Internal error: job[%zu].state=%d ", 
	       i, jobs[i].state);
      }
      printf("%s\n", jobs[i].commandLine);
    }
  }
}

/* clearJob - Clear the entries in a job struct */
void clearJob(job_t *job) {
  job->pid = 0;
  job->jid = 0;
  job->state = kUndefined;
  job->commandLine[0] = '\0';
}

/* initJobs - Initialize the job list */
void initJobs(job_t jobs[]) {
  for (size_t i = 0; i < kMaxJobs; i++)
    clearJob(&jobs[i]);
}

/* getMaxJobID - Returns largest allocated job ID */
int getMaxJobID(job_t jobs[]) {
  int max = jobs[0].jid;
  for (size_t i = 1; i < kMaxJobs; i++) {
    if (jobs[i].jid > max) {
      max = jobs[i].jid;
    }
  }
  
  return max;
}

/* addJob - Add a job to the job list */
bool addJob(job_t jobs[], pid_t pid, int state, const char *commandLine) {  
  if (pid < 1) return false;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == 0) {
      jobs[i].pid = pid;
      jobs[i].state = state;
      jobs[i].jid = nextJobID++;
      if (nextJobID > kMaxJobID)
	nextJobID = 1;
      strcpy(jobs[i].commandLine, commandLine);
      if (verbose) {
	printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].commandLine);
      }
      return true;
    }
  }

  printf("Tried to create too many jobs\n");
  return false;
}

/* deleteJob - Delete a job whose PID=pid from the job list */
bool deleteJob(job_t jobs[], pid_t pid)  {
  if (pid < 1) return false;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      clearJob(&jobs[i]);
      nextJobID = getMaxJobID(jobs) + 1;
      return true;
    }
  }
  return false;
}

/* getFGJobPID - Return PID of current foreground job, 0 if no such job */
pid_t getFGJobPID(job_t jobs[]) {
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].state == kForeground) {
      return jobs[i].pid;
    }
  }
  return 0;
}

/* getJobPID  - Find a job (by PID) on the job list */
job_t *getJobByPID(job_t jobs[], pid_t pid) {
  if (pid < 1) return NULL;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      return &jobs[i];
    }
  }
  return NULL;
}

/* getJobJID  - Find a job (by JID) on the job list */
job_t *getJobByJID(job_t jobs[], int jid) {
  if (jid < 1) return NULL;
  for (int i = 0; i < kMaxJobs; i++) {
    if (jobs[i].jid == jid) {
      return &jobs[i];
    }
  }
  return NULL;
}

/* getJIDFromPID - Map process ID to job ID */
int getJIDFromPID(pid_t pid) {
  if (pid < 1) return 0;
  for (size_t i = 0; i < kMaxJobs; i++) {
    if (jobs[i].pid == pid) {
      return jobs[i].jid;
    }
  }
  return 0;
}
