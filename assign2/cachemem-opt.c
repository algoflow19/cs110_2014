/**
 * cachemem.c  -  This module allocates the memory for caches. 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h> // for PRIu64

#include <sys/mman.h>

#include "cachemem.h"
#include "diskimg.h"

#define MAX_FILES 64
/*
 * 64 fds can be active in the open file table
 * Each inode corresponding to those 64 fds has 8 sector indexes in its inode structure
 * We are going to allocate 8 protected cache lines for those fds (most of which could be
 * directories) so that they to get a cache hit after the recursive function reaches
 * their level
 */
#define PROTECTED_CACHE_LINES 512
#define INODE_SECTOR_INDEXES 8

int cacheMemSizeInKB;
void *cacheMemPtr;

typedef struct cache {
    int sector;
    int dirty;
    int allocated;
    char buf[DISKIMG_SECTOR_SIZE];
}cache_line;
/**
 * Allocate memory of the specified size for the data cache optimizations
 * Return -1 on error, 0 on success. 
 */

int CacheMem_Init(int sizeInKB) {
  /**
   * Size needs to be not negative or too big and 
   * multiple of the 4KB page size 
   */
  if ((sizeInKB < 0) || (sizeInKB > (CACHEMEM_MAX_SIZE/1024)) || (sizeInKB % 4)) {
    fprintf(stderr, "Bad cache size %d\n", sizeInKB);
    return -1;
  }

  void *memPtr = mmap(NULL, 1024 * sizeInKB, PROT_READ | PROT_WRITE, 
		      MAP_PRIVATE|MAP_ANON, -1, 0);
  if (memPtr == MAP_FAILED) {
    perror("mmap");
    return -1;
  }

  cacheMemSizeInKB = sizeInKB;
  cacheMemPtr = memPtr;
  return 0;
}

void * get_protected_cache_block_for_fd(int fd) {
    char* start = (char *)cacheMemPtr;
    int block_in_bytes = INODE_SECTOR_INDEXES * sizeof(cache_line); 
    return (start + (block_in_bytes * fd));

}

void * get_unprotected_cache_block() {
    cache_line* start = (cache_line *)cacheMemPtr;
    return &start[PROTECTED_CACHE_LINES];
}

void invalidate_inode_sectors_content_in_protectedcache(int fd) {
    cache_line* start = get_protected_cache_block_for_fd(fd);
    for (int block_index = 0; block_index < INODE_SECTOR_INDEXES; block_index++) {
        start[block_index].dirty = 1;
        start[block_index].allocated = 0;
    }
}


int max_cache_line() {
    return ((cacheMemSizeInKB * 1024) / sizeof(cache_line));
}

cache_line *get_cache_line_for_sector(int sectornum) {
    int range = max_cache_line()  - PROTECTED_CACHE_LINES ; 
    int index = sectornum % range;
    cache_line *start = (cache_line *)get_unprotected_cache_block();
    return &start[index];
}

void prefetch_inode_sectors_content_in_protectedcache(struct unixfilesystem *unixfs, int fd, struct inode *inp) {
    int size = inode_getsize(inp);
    int required_blocks = size / DISKIMG_SECTOR_SIZE;
    if ((required_blocks * DISKIMG_SECTOR_SIZE) < size)
        required_blocks++;
    if (required_blocks > INODE_SECTOR_INDEXES)
        required_blocks = INODE_SECTOR_INDEXES;
    cache_line* start = get_protected_cache_block_for_fd(fd);
    for (int block_index = 0; block_index < required_blocks; block_index++) {
        int dereferenced_disk_block = inp->i_addr[block_index];
        start[block_index].sector = dereferenced_disk_block;
        start[block_index].dirty = 0;
        start[block_index].allocated = 1;
        diskimg_bypass_cache_read_sector(unixfs->dfd, dereferenced_disk_block, &start[block_index].buf);
    }
}

int is_sector_in_protected_cache(int sectornum, void *buf) {
    cache_line* start = get_protected_cache_block_for_fd(0);
    for (int index = 0; index < PROTECTED_CACHE_LINES; index++) {
        if ((start[index].sector == sectornum) && (start[index].dirty == 0) && (start[index].allocated == 1)) {
            memcpy(buf, (const void *)&start[index].buf, DISKIMG_SECTOR_SIZE);
            return 1;
        }
    }
    return -1;
}

int is_sector_in_unprotected_cache(int sectornum, void *buf) {
    cache_line * cached_sector = get_cache_line_for_sector(sectornum);
    if ((cached_sector->sector == sectornum) && (cached_sector->dirty == 0) && (cached_sector->allocated == 1)) {
        memcpy(buf, (const void *)&cached_sector->buf, DISKIMG_SECTOR_SIZE);
        return 1;
    }
    return -1;
}

int fetch_sector_in_cache(int sectornum, void *buf) {
   if (is_sector_in_protected_cache(sectornum, buf) == 1)
        return 1;
    if (is_sector_in_unprotected_cache(sectornum, buf) == 1)
        return 1;
    return -1;
}

void save_sector_in_cache(int sectornum, void *buf) {
    cache_line* cached_sector = get_cache_line_for_sector(sectornum);
    cached_sector->sector = sectornum;
    cached_sector->dirty = 0;
    cached_sector->allocated = 1;
    memcpy(&cached_sector->buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
}

