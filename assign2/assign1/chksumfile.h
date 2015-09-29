#ifndef _CHKSUMFILE_H_
#define _CHKSUMFILE_H_

#include "unixfilesystem.h"
#include "inode.h"

#define CHKSUMFILE_SIZE 20   
#define CHKSUMFILE_STRINGSIZE ((2*CHKSUMFILE_SIZE)+1)

/**
 * Computes the checksum of a inumber.  Assumes chksum arguments points to a
 * CHKSUMFILE_SIZE byte array.  Returns the length of the checksum, or -1 if
 * it encounters an error.
 */
int chksumfile_byinumber(struct unixfilesystem *fs, int inumber, void *chksum);

/**
 * Optimized version of chksumfile_byinumber. 
 * Gets inode pointer and the return value of inode_iget passed as argument.
 */
int optimized_chksumfile_byinode(struct unixfilesystem *fs, void *chksum, struct inode *inp, int inode_iget_ret);

/**
 * Compute the checksum of the specified pathname.  Assumes chksum points to a
 * CHKSUMFILE_SIZE byte array. Returns the length of the checksum or -1 if
 * it encounters an error.
 */
int chksumfile_bypathname(struct unixfilesystem *fs, const char *pathname, void *chksum);

/**
 * Converts a checksum into a string that can be printed.  Assumes
 * that outstring is CHKSUMFILE_STRINGSIZE in size.
 */
void chksumfile_cvt2string(void *chksum, char *outstring);

/**
 * Compares two checksums, returning 1 if they're the same and 0 otherwise.
 */
int chksumfile_compare(void *chksum1, void *chksum2);

#endif // _CHKSUMFILE_H_
