/**
 * File: tsh-state.h
 * -----------------
 * Encapsulates the global variables that should
 * be accessible and shared across modules.
 */

#ifndef _tsh_state_
#define _tsh_state_

#include "tsh-constants.h"
#include "tsh-jobs.h"

extern bool showPrompt;
extern bool verbose;
extern job_t jobs[kMaxJobs];

#endif
