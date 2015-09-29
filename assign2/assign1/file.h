#ifndef _FILE_H_
#define _FILE_H_

#include "unixfilesystem.h"
#include "inode.h"

/**
 * Fetches the specified file block from the specified inode.
 * Returns the number of valid bytes in the block, -1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNo, void *buf); 
int file_getblock_optimized(struct unixfilesystem *fs, int blockNum, void *buf, struct inode *inp, int inode_iget_ret); 

#endif // _FILE_H_
