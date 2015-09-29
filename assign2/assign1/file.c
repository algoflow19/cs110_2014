#include <stdio.h>
#include <assert.h>
#include "file.h"
#include "inode.h"
#include "diskimg.h"
#include "../cachemem.h"

/*
 * Helper Functions
 */
static inline int validate_file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf);

/*
 * Function : validate_file_getblock
 * Usage : if ((validate_file_getblock(fs, inumber, blockNum, buf)) == SUCCESS)
 * -------------------------------------------------------------------------------
 *  This function validates the inputs for file_getblock and returns 0/SUCCESS
 *  if they are valid else -1/FAILURE
 */
static inline int validate_file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    if ((fs) && (buf) &&
            /* inumber range 1 - fs->superblock.s_isize * INODES_IN_SECTOR inclusive */
            /* Confirmed the range with TAs */
        ((inumber > 0) && (inumber <= (fs->superblock.s_isize * INODES_IN_SECTOR))) &&
        ((blockNum >= MIN_INODE_BLOCKNUM) && (blockNum < MAX_INODE_BLOCKNUM)))
        return SUCCESS;
    return FAILURE;
}

/*
 * Function : validate_file_getblock_optimized
 * Usage : if ((validate_file_getblock_optimized(fs, blockNum, buf, inp)) == SUCCESS)
 * -------------------------------------------------------------------------------
 *  This function validates the inputs for file_getblock_optimized and returns 0/SUCCESS
 *  if they are valid else -1/FAILURE
 */
static inline int validate_file_getblock_optimized(struct unixfilesystem *fs, int blockNum, void *buf, struct inode *inp) {
    if ((fs) && (buf) && (inp) &&
        ((blockNum >= MIN_INODE_BLOCKNUM) && (blockNum < MAX_INODE_BLOCKNUM)))
        return SUCCESS;
    return FAILURE;
}


/*
 * Function : file_getblock_optimized
 * Usage : int valid_bytes = file_getblock(fs, dirinumber, blockNum, entries, inp, inode_iget_ret);
 * --------------------------------------------------------------------------------------------------
 *  This function fetches the specified disk block from the specified inode passed as argument.
 *  Returns the number of valid bytes in the block, FAILURE/-1 on error.
 */
int file_getblock_optimized(struct unixfilesystem *fs, int blockNum, void *buf, struct inode *inp, int inode_iget_ret) {
    if ((validate_file_getblock_optimized(fs, blockNum, buf, inp)) == SUCCESS) {
        if (inode_iget_ret < 0)
            return FAILURE;

        int read_bytes;

        // Reading from the disk
        int disk_block = inode_indexlookup(fs, inp, blockNum);
        if ((read_bytes = diskimg_readsector(fs->dfd, disk_block, buf)) != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, " Disk read failed, fs 0x%p disk_block %d inp 0x%p blockNum %d buf 0x%p inode_iget_ret %d, returning -1\n", fs, disk_block, inp, blockNum, buf, inode_iget_ret);
            return FAILURE;
        }
        int size = inode_getsize(inp);

        // When file size is not a multiple of 512, we need to lower the
        // read bytes accordingly.
        if (size < ((blockNum + 1) * DISKIMG_SECTOR_SIZE)) {
            read_bytes = size % DISKIMG_SECTOR_SIZE;
        }
        return read_bytes;
    } else {
        fprintf(stderr, "file_getblock_optimized(blockNum = %d, fs = 0x%p, buf = 0x%p, inp = 0x%p, inode_iget_ret = %d)   validation failed. returning -1\n", blockNum, fs, buf, inp, inode_iget_ret);
        return FAILURE;
    }
}



/*
 * Function : file_getblock
 * Usage : int valid_bytes = file_getblock(fs, dirinumber, blockNum, entries);
 * -----------------------------------------------------------------------------
 *  This function fetches the specified disk block from the specified inode.
 *  Returns the number of valid bytes in the block, FAILURE/-1 on error.
 */
int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
    if ((validate_file_getblock(fs, inumber, blockNum, buf)) == SUCCESS) {
        struct inode in;
        int read_bytes;

        //Fetching inode from inumber
        int err = inode_iget(fs, inumber, &in);
        if (err < 0) {
            return FAILURE;
        }

        // Getting disk block
        int disk_block = inode_indexlookup(fs, &in, blockNum);

        // Reading from the disk
        if ((read_bytes = diskimg_readsector(fs->dfd, disk_block, buf)) != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, " Disk read failed, fs 0x%p disk_block %d inp 0x%p blockNum %d buf 0x%p, returning -1\n", fs, disk_block, &in, blockNum, buf);
            return FAILURE;
        }
        int size = inode_getsize(&in);
        // When file size is not a multiple of 512, we need to lower the
        // read bytes accordingly.
        if (size < ((blockNum + 1) * DISKIMG_SECTOR_SIZE))
            read_bytes = size % DISKIMG_SECTOR_SIZE;
        return read_bytes;
    } else {
        fprintf(stderr, "file_getblock(inumber = %d, blockNum = %d, fs = 0x%p, buf 0x%p) validation failed. returning -1\n", inumber, blockNum, fs, buf);
        return FAILURE;  
    }
}
