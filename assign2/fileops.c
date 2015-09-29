/**
 * fileops.c  -  This module provides an Unix like file absraction
 * on the assign1 file system access code
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include "fileops.h"
#include "assign1/pathname.h"
#include "assign1/unixfilesystem.h"
#include "diskimg.h"
#include "assign1/inode.h"
#include "assign1/file.h"
#include "assign1/chksumfile.h"
#include "cachemem.h"

#define MAX_FILES 64
#define PREFETCHED_FILE_CONTENTS 1

static uint64_t numopens = 0;
static uint64_t numreads = 0;
static uint64_t numgetchars = 0;
static uint64_t numisfiles = 0;

/*
 * Structure of prefetched file content
 */
typedef struct file_disk_content {
    /* Used for storing the return value when prefetch happens */
    int read_bytes;
    /* Disk block content of file */
    char buf[512];
} file_content;


/**
 * Table of open files.
 */
static struct {
  char *pathname;    // absolute pathname NULL if slot is not used.
  int  cursor;       // Current position in the file
  /*
   * Optimization to store inode and inumber of open fd
   */
  int inumber;
  struct inode in;
  void *inp;
  int inode_iget_ret;
  /*
   * Max blockum that has been currently prefetched
   */
  int max_blocknum_in_store;
  /*
   * Upto 10 prefetched file content blocks
   */
  file_content content[PREFETCHED_FILE_CONTENTS];
} openFileTable[MAX_FILES];

static struct unixfilesystem *unixfs;

/**
 * Initialize the fileops module for the specified disk.
 */
void *Fileops_init(char *diskpath) {
  memset(openFileTable, 0, sizeof(openFileTable));
  int fd = diskimg_open(diskpath, 1);
  if (fd < 0) {
    fprintf(stderr, "Can't open diskimagePath %s\n", diskpath);
    return NULL;
  }

  unixfs = unixfilesystem_init(fd);
  if (unixfs == NULL) {
    diskimg_close(fd);
    return NULL;
  }
  return unixfs;
}

/**
 * Open the specified absolute pathname for reading. Returns -1 on error;
 */
int Fileops_open(char *pathname) {
  numopens++;
  struct inode in;
  int inumber = pathname_lookup(unixfs,pathname);
  if (inumber < 0) {
    return -1; // File not found
  }

  if ((inode_iget(unixfs, inumber,&in)) < 0) {
      return -1; // Inode node found
  }

  int fd;
  for (fd = 0; fd < MAX_FILES; fd++) {
    if (openFileTable[fd].pathname == NULL) break;
  }
  if (fd >= MAX_FILES) {
    return -1;  // No open file slots
  }
  openFileTable[fd].pathname = strdup(pathname); // Save our own copy
  openFileTable[fd].cursor = 0;
  openFileTable[fd].inumber = inumber;
  openFileTable[fd].in = in;
  openFileTable[fd].inp = &in;
  openFileTable[fd].inode_iget_ret = 0;
  /*
   * max_blocknum_in_store is initialized to -1 so that
   * it induces prefetch
   */
  openFileTable[fd].max_blocknum_in_store = -1;
  return fd;
}

/*
 * Preficate function that tells if the next set of
 * upto 10 file blocks needs to be prefetched.
 */
static int prefetch_needed(int fd, int blockNo) {
    if (openFileTable[fd].max_blocknum_in_store < blockNo)
        return 1;
    return -1;
}

/*
 * Prefetches content of upto 10 disk blocks.
 * ---------------------------------------------
 * As scan tree and index will read every file and directory present in the
 * disk image, it makes perfect sense to prefetch. There wont be any
 * unnecessary prefectch in this model, all prefetched content will be utilized.
 *
 * The modest 1MB direct mapped cache could get overwrriten by chksum
 * by inumber (pathstore, scan file) for large files, therefore save upto
 * 10 blocks of file content that would be inevitably utilized by
 * scantree and index recursive function, so that we won't have to
 * redo I/O or scramble for disk sector blocks in cache when control 
 * reaches back to upper levels (directories)
 * of pathname based recursion.
 */
static void prefetch_file_contents(int fd) {
    int size = inode_getsize(&openFileTable[fd].in);
    int numBlocks  = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    int next_prefetch_block = openFileTable[fd].max_blocknum_in_store + 1;

    // Calculate the number of blocks to be prefetched.
    // Does not prefetch blocks that are beyond the size of the file.
    if ((numBlocks - next_prefetch_block) <= PREFETCHED_FILE_CONTENTS) {
        openFileTable[fd].max_blocknum_in_store = numBlocks - 1;
    } else {
        openFileTable[fd].max_blocknum_in_store = next_prefetch_block + PREFETCHED_FILE_CONTENTS - 1;
    }
    int index = 0;
    for (; next_prefetch_block <= openFileTable[fd].max_blocknum_in_store; next_prefetch_block++, index++) {
        int read_bytes = file_getblock_optimized(unixfs, next_prefetch_block, &openFileTable[fd].content[index].buf, &openFileTable[fd].in, openFileTable[fd].inode_iget_ret);
        /* Storing the return value of file_getblock */
        openFileTable[fd].content[index].read_bytes = read_bytes;
    }
}

/*
 * Gets prefetched file content
 * The content should have been prefetched.
 * Used in conjunction with prefetch_needed and prefetch_file_contents
 */
static int prefetched_file_content(int fd, int blockNo, unsigned char* buf) {
    /* Very simple lookup */
    int index = blockNo % PREFETCHED_FILE_CONTENTS;
    memcpy(buf, &openFileTable[fd].content[index].buf, DISKIMG_SECTOR_SIZE);
    return openFileTable[fd].content[index].read_bytes;
}

/**
 * Fetch the next character from the file. Return -1 if at end of file.
 */
int Fileops_getchar(int fd) {
  unsigned char buf[DISKIMG_SECTOR_SIZE];
  int bytesMoved;
  int err, size;
  int blockNo, blockOffset;

  numgetchars++;

  if (openFileTable[fd].pathname == NULL)
    return -1;  // fd not opened.

  if (openFileTable[fd].inumber < 0)
      return -1;

  if (!(openFileTable[fd].in.i_mode & IALLOC))
      return -1;

  size = inode_getsize(&openFileTable[fd].in);

  if (openFileTable[fd].cursor >= size) return -1; // Finished with file

  blockNo = openFileTable[fd].cursor / DISKIMG_SECTOR_SIZE;
  blockOffset =  openFileTable[fd].cursor % DISKIMG_SECTOR_SIZE;

  if (prefetch_needed(fd, blockNo) == 1)
      prefetch_file_contents(fd);

  bytesMoved = prefetched_file_content(fd, blockNo, buf);

  if (bytesMoved < 0) {
    return -1;
  }
  assert(bytesMoved > blockOffset);

  openFileTable[fd].cursor += 1;

  return (int)(buf[blockOffset]);
}

/**
 * Implement the Unix read system call. Number of bytes returned.  Return -1 on
 * err.
 */
int Fileops_read(int fd, char *buffer, int length) {
  numreads++;
  int i;
  for (i = 0; i < length; i++) {
    int ch = Fileops_getchar(fd);
    if (ch == -1) break;
    buffer[i] = ch;
  }
  return i;
}

/**
 * Return the current position in the file.
 */
int Fileops_tell(int fd) {
  if (openFileTable[fd].pathname == NULL)
    return -1;  // fd not opened.
  return openFileTable[fd].cursor;
}

/**
 * Close the files - return the resources
 */

int Fileops_close(int fd) {
  if (openFileTable[fd].pathname == NULL)
    return -1;  // fd not opened.
  free(openFileTable[fd].pathname);
  openFileTable[fd].pathname = NULL;
  /*
   *
   * Erases the inode cache contents pertaining to this open fd / inp
   *
   */
  erase_sector_in_inode_cache(openFileTable[fd].inp);
  return 0;
}

/**
 * Return true if specified pathname is a regular file.
 */
int Fileops_isfile(char *pathname) {
  numisfiles++;
  int inumber = pathname_lookup(unixfs, pathname);
  if (inumber < 0) {
    return 0;
  }

  struct inode in;
  int err = inode_iget(unixfs, inumber, &in);
  if (err < 0) return 0;

  if (!(in.i_mode & IALLOC) || ((in.i_mode & IFMT) != 0)) {
    // Not allocated or not a file
    return 0;
  }

  return 1; // Must be a file
}

/*
 * Optimized version of Fileops_isfile
 *  Also Gets inumber and inode corresponding to pathname
 * Returns -2 if inode cannot be fetched, inumber value is set.
 * Returns -1 if inumber cannot be fetched from pathname, inode and inumber are not set.
 * Returns 1, if pathname is a file, sets inode and inumber
 *
 * Eliminates 3 pathname_lookup calls in the following functions :
 * Pathstore_path via checksum by pathname
 * Scan_TreeAndIndex via fileops_open
 * Scan_File via fileops_open
 *
 * Eliminates 4 inode_iget calls in the following functions :
 * Pathstore_path via checksum by pathname
 * Scan_TreeAndIndex via fileops_open
 * Scan_File via fileops_open
 * checksum_byinumber via checksum by pathname
 *
 */
int optimized_Fileops_isfile(int inumber, struct inode *inp, int *inode_iget_ret) {
  numisfiles++;
  (*inode_iget_ret) = inode_iget(unixfs, inumber, inp);
  if ((*inode_iget_ret) < 0) return 0;


  if (!(inp->i_mode & IALLOC) || ((inp->i_mode & IFMT) != 0)) {
      // Not allocated or not a file
      return 0;
  }

  return 1; // Must be a file
}

/*
 *
 * Optimized fileops_open to utilize the inode, inp and inumber found in optimized filops_isfile
 *
 */
int optimized_Fileops_open(char *pathname, int inumber, struct inode *inp, int inode_iget_ret) {
  assert(inp != NULL);
  numopens++;
  if (inumber < 0) {
    return -1; // File not found
  }

  if (inode_iget_ret < 0) 
    return -1;

  int fd;
  for (fd = 0; fd < MAX_FILES; fd++) {
    if (openFileTable[fd].pathname == NULL) break;
  }

  if (fd >= MAX_FILES) {
    return -1; // No open file slots
  }

  openFileTable[fd].pathname = strdup(pathname); // Save our own copy
  openFileTable[fd].cursor = 0;
  openFileTable[fd].inumber = inumber;
  openFileTable[fd].in = *inp;
  openFileTable[fd].inp = (void *)inp;
  openFileTable[fd].inode_iget_ret = 0;

  /*
   * max_blocknum_in_store is initialized to -1 so that
   * it induces prefetch
   */
  openFileTable[fd].max_blocknum_in_store = -1;
  return fd;
}

void Fileops_Dumpstats(FILE *file) {
  fprintf(file,
          "Fileops: %"PRIu64" opens, %"PRIu64" reads, "
          "%"PRIu64" getchars, %"PRIu64 " isfiles\n",
          numopens, numreads, numgetchars, numisfiles);
}

