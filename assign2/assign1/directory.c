#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <search.h>

/*
 * Macros
 */
#define NUM_DIR_ENTRIES_IN_BLOCK 32

/*
 * Helper functions
 */
static inline int validate_directory_findname(struct unixfilesystem *fs, const char *name, int dirinumber, struct direntv6 *dirEnt);
int cmp_dir_entries(const void *addr1, const void *addr2);

/*
 * Function : validate_directory_findname
 * Usage : if ((validate_directory_findname(fs, name, dirinumber, dirEnt)) == SUCCESS)
 * -----------------------------------------------------------------------------------
 *  This function validates the inputs for directory_findname and returns 0/SUCCESS
 *  if they are valid else -1/FAILURE
 */
static inline int validate_directory_findname(struct unixfilesystem *fs, const char *name,
                int dirinumber, struct direntv6 *dirEnt) {
    /* inumber range 1 - fs->superblock.s_isize * INODES_IN_SECTOR inclusive */
    /* Confirmed the range with TAs */
    if ((fs) && (name) && (dirEnt) && ((dirinumber > 0)  && (dirinumber <= (fs->superblock.s_isize * INODES_IN_SECTOR))))
        return SUCCESS;
    return FAILURE;
}

/*
 * Function : cmp_dir_entries
 * Usage : found = lfind(&key, entries, &num_entries, dir_entry_size, cmp_dir_entries);
 * ------------------------------------------------------------------------------------
 *  Compare function used in linear find of directory entries
 */
int cmp_dir_entries(const void *addr1, const void *addr2) {
    const struct direntv6 *key = (const struct direntv6 *)addr1;
    const struct direntv6 *entry = (const struct direntv6 *)addr2;
    return memcmp(key->d_name, entry->d_name, sizeof(key->d_name));
}

/*
 * Function : directory_findname
 * Usage : if ((directory_findname(fs, sub_path, inumber, &member)) == SUCCESS)
 * ----------------------------------------------------------------------------
 * This function looks up the specified name in the specifien directory inumber.
 * If found, return the directory entry in space addressed by dirEnt.  Returns 0
 * /SUCCESS on success and -1/FAILURE on failure
 */
int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
    if ((validate_directory_findname(fs, name, dirinumber, dirEnt)) == SUCCESS) {
        struct inode dirin;
        // Fetching inode form inumber.
        int inode_iget_ret = inode_iget(fs, dirinumber, &dirin);
        if (inode_iget_ret < 0) {
            return FAILURE;
        }
        // Sanity checks to verifiy if the inode is indeed a directory
        if (!(dirin.i_mode & IALLOC) || ((dirin.i_mode & IFMT) != IFDIR)) {
            return FAILURE;
        }
        struct direntv6 key;
        /* Directory entry names can be upto 13 characters long */
        strncpy((char *)&key.d_name, name, sizeof(key.d_name) - 1);
        // Appending NULL at the last spot - Just in case the name was 13 character long
        key.d_name[sizeof(key.d_name) - 1] = '\0';
        size_t dir_entry_size = sizeof(key);
        int size = inode_getsize(&dirin);
        // Loop till we have perused every block in this inode
        for (int offset = 0; offset < size; offset += DISKIMG_SECTOR_SIZE) {
            // Preparing directory entry data structure
            struct direntv6 entries[NUM_DIR_ENTRIES_IN_BLOCK];
            // Inode block indexes spill over for ever DISKIMG_SECTOR_SIZE bytes */
            int blockNum = offset / DISKIMG_SECTOR_SIZE;
            int valid_bytes = file_getblock_optimized(fs, blockNum, entries, &dirin, inode_iget_ret);
            if (valid_bytes < 0)
                return FAILURE;
            // Calculate number of entries
            size_t num_entries = valid_bytes / dir_entry_size;
            // Using lfind utility to quickly search for the key.
            struct direntv6 * found = lfind(&key, entries, &num_entries, dir_entry_size, cmp_dir_entries);
            if ((found)) {
                // Copy the found directory entry and return
                // Optimization for CPU time
                *dirEnt = *found; 
                return SUCCESS;
            }
        }
        return FAILURE;
    } else  {
        fprintf(stderr, "directory_lookupname(name=%s dirinumber=%d fs = 0x%p direntv6 = 0x%p) validation failed. returning -1\n", name, dirinumber, fs, dirEnt); 
        return FAILURE;
    }
}

/*
 * Function : validate_directory_findname_optimized
 * Usage : if ((validate_directory_findname_optimized(fs, name, dirinp, 
 *              inode_iget_ret, dirEnt)) == SUCCESS)
 * ------------------------------------------------------------------------
 *  This function validates the inputs for directory_findname and returns 0/SUCCESS
 *  if they are valid else -1/FAILURE
 */
static inline int validate_directory_findname_optimized(struct unixfilesystem *fs, const char *name,
                struct inode *dirinp, struct direntv6 *dirEnt) {
    if ((fs) && (name) && (dirEnt) && (dirinp))
        return SUCCESS;
    return FAILURE;
}

int directory_findname_optimized(struct unixfilesystem *fs, const char *name, struct inode *dirinp, int inode_iget_ret, struct direntv6 *dirEnt) {
    if (validate_directory_findname_optimized(fs, name, dirinp, dirEnt) == SUCCESS) {
        if (inode_iget_ret < 0)
            return FAILURE;

        if (!(dirinp->i_mode & IALLOC) || ((dirinp->i_mode & IFMT) != IFDIR)) {
            return FAILURE;
        }

        struct direntv6 key;
        /* Directory entry names can be upto 13 characters long */
        strncpy((char *)&key.d_name, name, sizeof(key.d_name) - 1);
        // Appending NULL at the last spot - Just in case the name was 13 character long
        key.d_name[sizeof(key.d_name) - 1] = '\0';
        size_t dir_entry_size = sizeof(key);
        int size = inode_getsize(dirinp);
        // Loop till we have perused every block in this inode
        for (int offset = 0; offset < size; offset += DISKIMG_SECTOR_SIZE) {
            // Preparing directory entry data structure
            struct direntv6 entries[NUM_DIR_ENTRIES_IN_BLOCK];
            // Inode block indexes spill over for ever DISKIMG_SECTOR_SIZE bytes */
            int blockNum = offset / DISKIMG_SECTOR_SIZE;
            int valid_bytes = file_getblock_optimized(fs, blockNum, entries, dirinp, inode_iget_ret);
            if (valid_bytes < 0)
                return FAILURE;
            // Calculate number of entries
            size_t num_entries = valid_bytes / dir_entry_size;
            // Using lfind utility to quickly search for the key.
            struct direntv6 * found = lfind(&key, entries, &num_entries, dir_entry_size, cmp_dir_entries);
            if ((found)) {
                // Copy the found directory entry and return
                // Optimization for CPU time
                *dirEnt = *found;
                return SUCCESS;
            }
        }
        return FAILURE;
    } else  {
        fprintf(stderr, "directory_findname_optimized(name=%s dirinp =0x%p fs = 0x%p direntv6 = 0x%p inode_iget_ret = %d) validation failed. returning -1\n", name, dirinp, fs, dirEnt, inode_iget_ret);
        return FAILURE;
    }
}
