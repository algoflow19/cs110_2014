/**
 * File: mr-env.cc
 * ---------------
 * Provides the implementation of the environment-variable-oriented
 * functions exported by mr-env.h.
 */

#include "mr-env.h"
#include <cstdlib> // for getenv
using namespace std;

static const char *getFirstNonNullValue(const char *const vars[]) {
  for (size_t i = 0; vars[i] != NULL; i++) {
    const char *value = getenv(vars[i]);
    if (value != NULL) return value;
  }

  return NULL;
}

/**
 * Function: getUser
 * -----------------
 * Returns the SUNet ID of the logged in user.  Implementation
 * accommodates the possibility that user may be running any one
 * of several different shells (although I think all shells store
 * the current user under "USER".
 */
static const char *const kUserEnvVariables[] = {"USER", NULL};
const char *getUser() {
  return getFirstNonNullValue(kUserEnvVariables);
}

/**
 * Function: getHost
 * -----------------
 * Returns the hostname where the server is running (e.g. "myth15",
 * but without the ".stanford.edu".  Note that different shells
 * use different environment variable names, so we need to look
 * under both "HOST" and "HOSTNAME".
 */
static const char *const kHostEnvVariables[] = {"HOST", "HOSTNAME", NULL};
const char *getHost() {
  return getFirstNonNullValue(kHostEnvVariables);
}

/**
 * Function: getCurrentWorkingDirectory
 * ------------------------------------
 * Returns the current working directory, allowing for the possibility
 * that the cwd comes from one of several different environment variables
 * (though I only know of PWD right now)
 */
static const char *const kPWDEnvVariables[] = {"PWD", NULL};
const char *getCurrentWorkingDirectory() {
  return getFirstNonNullValue(kPWDEnvVariables);
}
