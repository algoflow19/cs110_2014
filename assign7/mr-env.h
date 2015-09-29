/**
 * File: mr-env.h
 * --------------
 * Provides a collection of obvious functions that surface
 * the values of environment variables pertinent to the
 * execution of the MapReduce (mr) system.
 */

#pragma once

/**
 * Function: getUser
 * -----------------
 * Returns the SUNet ID of the person running the server.
 */
const char *getUser();

/**
 * Function: getHost
 * -----------------
 * Returns the name of the machine where the server is running.
 */
const char *getHost();

/**
 * Function: getCurrentWorkingDirectory
 * ------------------------------------
 * Returns the absolute pathname of the current
 * working directory.
 */
const char *getCurrentWorkingDirectory();


