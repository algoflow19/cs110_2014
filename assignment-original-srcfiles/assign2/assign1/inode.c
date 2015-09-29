/* Header files */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "inode.h"
#include "diskimg.h"

/*
 * Helper functions
 */
static inline unsigned int calculate_sector_index_from_inumber(int inumber);
static inline unsigned int calculate_inode_offset_in_sector(int inumber);
static inline int validate_indexlookup(struct unixfilesystem *fs, struct inode *inp, int   blockNum);
static int find_disk_block_for_small_files(struct inode *inp, int blockNum);
static int find_disk_block_from_double_indirection_block(struct unixfilesystem *fs, int    blockNum, uint16_t *double_indirection_block);
static int find_disk_block_for_large_files(struct unixfilesystem *fs, struct inode *inp,   int blockNum);

/*
 * Simple function to calculate sector index
 */
static inline unsigned int calculate_sector_index_from_inumber(int inumber) {
    /* As inumbers start with 1 */
    unsigned int actual_inumber_offset = (unsigned int) (inumber - ROOT_INUMBER);
    unsigned int index = INODE_START_SECTOR + (actual_inumber_offset / INODES_IN_SECTOR);
    return index;
}

/*
 * Simple function to calculate inode offset
 */
static inline unsigned int calculate_inode_offset_in_sector(int inumber) {
    unsigned int actual_inumber_offset = (unsigned int) (inumber - ROOT_INUMBER);
    return (actual_inumber_offset % INODES_IN_SECTOR);

}

/*
 * Simple function to validate inode_iget inputs
 */
static inline int validate_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    /* inumber range 1 - fs->superblock.s_isize * INODES_IN_SECTOR inclusive */
    /* Confirmed the range with TAs */
    if ((fs) && ((inumber > 0) && (inumber <= (fs->superblock.s_isize * INODES_IN_SECTOR))) && (inp))
        return SUCCESS;
    return FAILURE;
}

/* 
 * Simple function to validate indexlookup inputs
 */
static inline int validate_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if ((fs) && (inp) && ((blockNum >= MIN_INODE_BLOCKNUM) && (blockNum < MAX_INODE_BLOCKNUM)))
        return SUCCESS;
    return FAILURE;
}

/*
 * Function : inode_iget
 * Usage : int err = inode_iget(fs, inumber, &in);
 * ---------------------------------------------------
 * Fetches the specified inode from the filesystem.
 * Returns 0/SUCCESS 0 on success, -1/ERROR on error.
 */
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
    if (!(validate_iget(fs, inumber, inp))) {
        unsigned int sector_index = calculate_sector_index_from_inumber(inumber);
        struct inode inode_sector[INODES_IN_SECTOR];
        if (diskimg_readsector(fs->dfd, sector_index, inode_sector) != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, "Disk read failed, fs 0x%p sector index %u inumber=%d , returning -1\n", fs, sector_index, inumber);
            return FAILURE;
        }
        unsigned int inode_offset = calculate_inode_offset_in_sector(inumber);
        memcpy(inp, (const void *)&inode_sector[inode_offset], sizeof(struct inode));
        return SUCCESS;
    }
    return FAILURE;  
}

/*
 * Simple function to find disk block for small files <=4096 bytes
 */
static inline int find_disk_block_for_small_files(struct inode *inp, int blockNum) {
    if (blockNum < MAX_DIRECT_DISK_BLOCK_INDEXES_IN_INODE)
        return inp->i_addr[blockNum]; 
    return FAILURE;
}

/*
 * Fetches disk sector block by dereferencing from a double indirection block
 */
static int find_disk_block_from_double_indirection_block(struct unixfilesystem *fs, int blockNum, uint16_t *double_indirection_block) {
    int double_indirection_index = (blockNum / MAX_DISK_BLOCK_INDEXES_IN_BLOCK) - NUM_SINGLE_INDIRECTION_INDEXES;
    int dereferenced_disk_block = double_indirection_block[double_indirection_index];
    uint16_t single_indirection_block[MAX_DISK_BLOCK_INDEXES_IN_BLOCK];
    if (diskimg_readsector(fs->dfd, dereferenced_disk_block, single_indirection_block) != DISKIMG_SECTOR_SIZE) {
        fprintf(stderr, "Disk read failed, fs 0x%p dereferenced_disk_block %u  blockNum %d, returning -1\n", fs, dereferenced_disk_block, blockNum);
        return FAILURE;
    }
    int offset = blockNum % MAX_DISK_BLOCK_INDEXES_IN_BLOCK;
    return single_indirection_block[offset];
}

/*
 * Finds the disk sector block for large files which can have single and double
 * indirection block
 */
static int find_disk_block_for_large_files(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    int block_index;
    int disk_block;
    int dereferenced_disk_block;
    uint16_t block_content[MAX_DISK_BLOCK_INDEXES_IN_BLOCK];
    if (blockNum < STARTING_BLOCKNUM_IN_DOUBLY_INDIRECT_BLOCK) {
        // Single indirection blocks
        block_index = blockNum / MAX_DISK_BLOCK_INDEXES_IN_BLOCK; 
        dereferenced_disk_block = inp->i_addr[block_index];
        if (diskimg_readsector(fs->dfd, dereferenced_disk_block, block_content) != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, "Disk read failed, fs 0x%p dereferenced_disk_block %u inp 0x%p blockNum %d, returning -1\n", fs, dereferenced_disk_block, inp, blockNum);
            return FAILURE;
        }
        int offset = blockNum % MAX_DISK_BLOCK_INDEXES_IN_BLOCK;
        disk_block = block_content[offset];
    } else {
        // Handling double indirection block
        block_index = DOUBLY_INDIRECT_DISK_BLOCK_INDEX;
        dereferenced_disk_block = inp->i_addr[block_index];
        if (diskimg_readsector(fs->dfd, dereferenced_disk_block, block_content) != DISKIMG_SECTOR_SIZE) {
            fprintf(stderr, "Disk read failed, fs 0x%p dereferenced_disk_block %u inp 0x%p blockNum %d, returning -1\n", fs, dereferenced_disk_block, inp, blockNum);
            return FAILURE;
        }
        disk_block = find_disk_block_from_double_indirection_block(fs, blockNum, block_content);
    }
    return disk_block;
}

/*
 * Function : inode_indexlookup
 * Usage : int disk_block = inode_indexlookup(fs, &in, blockNum);
 * --------------------------------------------------------------------
 * Gets the location of the specified file block of the specified inode.
 * Returns the disk block number on success, -1/ERROR on error
 */
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
    if (!(validate_indexlookup(fs, inp, blockNum))) {
        int disk_block_num;
        // Check if the inode has indirection.
        if ((inp->i_mode & ILARG)) {
            disk_block_num = find_disk_block_for_large_files(fs, inp, blockNum);
        } else {
            disk_block_num = find_disk_block_for_small_files(inp, blockNum);
        }
        return disk_block_num;
    }
    return FAILURE;
}

int inode_getsize(struct inode *inp) {
    return ((inp->i_size0 << 16) | inp->i_size1); 
}
