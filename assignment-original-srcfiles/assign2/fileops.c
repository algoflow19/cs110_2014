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
#define INODE_SECTOR_INDEXES 10
#define PREFETCHED_FILE_CONTENTS 10

static uint64_t numopens = 0;
static uint64_t numreads = 0;
static uint64_t numgetchars = 0;
static uint64_t numisfiles = 0;


typedef struct file_disk_content {
    int read_bytes;
    char buf[512];
} file_content;


/**
 * Table of open files.
 */
static struct {
  char *pathname;    // absolute pathname NULL if slot is not used.
  int  cursor;       // Current position in the file
  int max_blocknum_in_store;
  int inumber;
  struct inode in;
  file_content content[PREFETCHED_FILE_CONTENTS];
} openFileTable[MAX_FILES];

static struct unixfilesystem *unixfs;

/*
void prefetch_inode_sectors_content_in_cache(int fd, struct inode *inp, inode_sectors_content *content) {
    int size = inode_getsize(inp);
    int required_blocks = size / DISKIMG_SECTOR_SIZE;
    if ((required_blocks * DISKIMG_SECTOR_SIZE) < size);
        required_blocks++;
    
    if (required_blocks > INODE_SECTOR_INDEXES)
        required_blocks = INODE_SECTOR_INDEXES;
//    int max_block_index = size / 512;
//    max_block_index = (max_block_index < INODE_SECTOR_INDEXES) : 
    for (int block_index = 0; block_index < required_blocks; block_index++) {
        int dereferenced_disk_block = inp->1_addr[block_index];
        diskimg_readsector(unixfs->dfd, dereferenced_disk_block, &content[block_index]);  
    }
}
*/

/*
void prefectch_file_contents(int start_blockno, int fd) {
  if (openFileTable[fd].pathname == NULL)
    return -1;  // fd not opened.

  if (openFileTable[fd].inumber < 0)
    return -1;

  if (!(openFileTable[fd].in.i_mode & IALLOC))
    return -1;

}
*/

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
  openFileTable[fd].max_blocknum_in_store = -1;
//  prefetch_inode_sectors_content_in_protectedcache(unixfs, fd, &in);
  return fd;
}

int prefetch_needed(int fd, int blockNo) {
    if (openFileTable[fd].max_blocknum_in_store < blockNo)
        return 1;
    return -1;
}

void prefetch_file_contents(int fd) {
    int size = inode_getsize(&openFileTable[fd].in);
    int numBlocks  = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
    int next_prefetch_block = openFileTable[fd].max_blocknum_in_store + 1;
    if ((numBlocks - next_prefetch_block) <= PREFETCHED_FILE_CONTENTS) {
        openFileTable[fd].max_blocknum_in_store = numBlocks - 1;
    } else {
        openFileTable[fd].max_blocknum_in_store = next_prefetch_block + PREFETCHED_FILE_CONTENTS - 1;
    }
    int index = 0;
    for (; next_prefetch_block <= openFileTable[fd].max_blocknum_in_store; next_prefetch_block++, index++) {
        int read_bytes = file_getblock(unixfs,openFileTable[fd].inumber, next_prefetch_block, &openFileTable[fd].content[index].buf);
        openFileTable[fd].content[index].read_bytes = read_bytes;
    }
}

int prefetched_file_content(int fd, int blockNo, unsigned char* buf) {
    int index = blockNo % PREFETCHED_FILE_CONTENTS;
    memcpy(buf, &openFileTable[fd].content[index].buf, DISKIMG_SECTOR_SIZE);
    return openFileTable[fd].content[index].read_bytes;
}

/**
 * Fetch the next character from the file. Return -1 if at end of file.
 */
int Fileops_getchar(int fd) {
//  int inumber;
//  struct inode in;
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
/*
  inumber = pathname_lookup(unixfs, openFileTable[fd].pathname);
  if (inumber < 0) {
    return inumber; // Can't find file
  }

  err = inode_iget(unixfs, inumber,&in);
  if (err < 0) {
    return err;
  }
  if (!(in.i_mode & IALLOC)) {
    return -1;
  }
*/
  size = inode_getsize(&openFileTable[fd].in);

  if (openFileTable[fd].cursor >= size) return -1; // Finished with file

  blockNo = openFileTable[fd].cursor / DISKIMG_SECTOR_SIZE;
  blockOffset =  openFileTable[fd].cursor % DISKIMG_SECTOR_SIZE;

  if (prefetch_needed(fd, blockNo) == 1)
      prefetch_file_contents(fd);

  bytesMoved = prefetched_file_content(fd, blockNo, buf);
  //if (blockNo < PREFETCHED_SECTOR_CONTENTS) {
 //   bytesMoved = 
//  } else {
//    bytesMoved = file_getblock(unixfs,openFileTable[fd].inumber,blockNo,buf);
//  }
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
  return 0;
}

/**
 * Return true if specified pathname is a regular file.
 */
int Fileops_isfile(char *pathname) {
  numisfiles++;
// /*
  int inumber = pathname_lookup(unixfs, pathname);
  if (inumber < 0) {
    return 0;
  }

  struct inode in;
  int err = inode_iget(unixfs, inumber, &in);
  if (err < 0) return 0;
//  */

  if (!(in.i_mode & IALLOC) || ((in.i_mode & IFMT) != 0)) {
    // Not allocated or not a file
    return 0;
  }
  return 1; // Must be a file
}

void Fileops_Dumpstats(FILE *file) {
  fprintf(file,
          "Fileops: %"PRIu64" opens, %"PRIu64" reads, "
          "%"PRIu64" getchars, %"PRIu64 " isfiles\n",
          numopens, numreads, numgetchars, numisfiles);
}

