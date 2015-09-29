/**
 * File: tsh-state.c
 * -----------------
 * Defines storage for and initializes the
 * global variables that must be accessible
 * across all files.
 */

#include "tsh-state.h"

/**
 * Global variables are normally verboten, but when
 * multiples modules need access to shared
 * state, it's common to rely on globals rather
 * than passing the address of some master
 * structure across every single function call
 * boundary.
 */

bool showPrompt = true;
bool verbose = false;
job_t jobs[kMaxJobs];
