#ifndef _DIRECTORY_H_
#define _DIRECTORY_H_

#include "unixfilesystem.h"
#include "direntv6.h"
#include "inode.h"

/**
 * Looks up the specified name (name) in the specified directory (dirinumber).  
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0 
 * on success and something negative on failure. 
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
                       int dirinumber, struct direntv6 *dirEnt);
int directory_findname_optimized(struct unixfilesystem *fs, const char *name, struct inode *dirinp, int inode_iget_ret, struct direntv6 *dirEnt);

#endif // _DIECTORY_H_
