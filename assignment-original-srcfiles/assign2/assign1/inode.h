#ifndef _INODE_H
#define _INODE_H

#include "unixfilesystem.h"

/*
 * Macros
 */
#define SUCCESS 0
#define FAILURE -1
#define INODES_IN_SECTOR 16 /* 16 32 byte inodes in a 512 byte sector */
#define MIN_INODE_BLOCKNUM 0
#define MAX_INODE_BLOCKNUM 32768 /* Max file size is determined by offset width in V6
                                    When offset width is 3, max file size is 2^24
                                    which can be accomodated in 32768 disk block/sectors
                                  */
#define MAX_INODE_BLOCKNUM_FOR_SMALLER_FILES 8 /* Upto 4096 bytes in 8 disk blocks */
#define STARTING_BLOCKNUM_IN_DOUBLY_INDIRECT_BLOCK 1792 /* First 7 indexes in inode block
                                                           array point to 7 indirect blocks
                                                           which are capable of indexing
                                                           256 disk blocks each. Hence the
                                                           blocknum that will be stored in
                                                           the 8th index will be 7 * 256
                                                         */
#define MAX_DISK_BLOCK_INDEXES_IN_BLOCK 256 /* 256 2 byte indexes in 512 byte sector */
#define MAX_DIRECT_DISK_BLOCK_INDEXES_IN_INODE 8 /* Upto 4096 bytes in 8 disk blocks */
#define DOUBLY_INDIRECT_DISK_BLOCK_INDEX 7
#define NUM_SINGLE_INDIRECTION_INDEXES 7


/**
 * Fetches the specified inode from the filesystem. 
 * Returns 0 on success, -1 on error.  
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp); 

/**
 * Gets the location of the specified file block of the specified inode.
 * Returns the disk block number on success, -1 on error.  
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum);

/**
 * Computes the size of an inode from its size0 and size1 fields.
 */
int inode_getsize(struct inode *inp);

#endif // _INODE_
