/**
 * File: tsh-signal.c
 * ------------------
 * Presents the implementation of the installSignalHandler
 * routine, which appears on page 752 of B&O, 2nd edition.
 */

#include <signal.h>
#include "tsh-signal.h"
#include "exit-utils.h"

handler_t installSignalHandler(int signum, handler_t handler) {
  struct sigaction action, old_action;  
  action.sa_handler = handler;  
  sigemptyset(&action.sa_mask); // block sigs of type being handled
  action.sa_flags = SA_RESTART; // restart syscalls if possible  
  exitIf(sigaction(signum, &action, &old_action) < 0,
	 1, stdout, "installSignalHandler failed.\n");
  return old_action.sa_handler;
}
