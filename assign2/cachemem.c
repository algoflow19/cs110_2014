/**
 * cachemem.c  -  This module allocates the memory for caches. 
 */

/*
 * Header files 
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

/*
 * Macros
 */
#define BYTES_IN_KB 1024
#define ALLOCATED 1
#define INODE_CACHE_LINES 36 /*
                              * The maximum depth of manyfiles.img is 14 
                              * Directories get 2, files get 8 blocks in INODE_CACHE
                              * And so a total of 36 cache lines
                              */
#define MINIMUM_INODE_CACHE_SPACE 256

/*
 * Globals
 */
int cacheMemSizeInKB;
void *cacheMemPtr;
static int cache_is_allocated;
static int inode_cache_is_enabled;

/*
 * Stripped down cache line strucure designed to increase number of cache blocks
 * Removed signature, diry / allocated flags
 */
typedef struct cache {
    int sector;
    char buf[DISKIMG_SECTOR_SIZE];
}cache_line;

/*
 * Special inode cache structure with inp key
 */
typedef struct inode_sectors {
    int sector;
    void *inp;
    /*
     *
     * Upper 16 bytes correspond to type file or directory
     * Lower correspond to indirection
     *
     */
    int typeandindirection;
    char buf[DISKIMG_SECTOR_SIZE];
}inode_cache_line;

/*
 * Helper functions
 */
static int max_cache_line();
static cache_line *get_cache_line_for_sector(int sectornum);
static int is_directory(int typeandindirection);
static int is_singly_indirected(int typeandindirection);

/*
 *
 * Predicate functions 
 *
 */
static int is_directory(int typeandindirection) {
    return (typeandindirection & 0xffff0000);
}

static int is_singly_indirected(int typeandindirection) {
    return ((typeandindirection & 0xffff) == 1);
}

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

  /*
   * memPtr points to a region of memory that is filled with 0s
   * As the first cache block will by default filled with sector 
   * number 0 - Superblock will not be cached.
   */
  cacheMemSizeInKB = sizeInKB;
  cacheMemPtr = memPtr;
  cache_is_allocated = ALLOCATED;
  if (cacheMemSizeInKB < MINIMUM_INODE_CACHE_SPACE)
      inode_cache_is_enabled = 0;
  else
      inode_cache_is_enabled = 1;
  return 0;
}

/*
 *
 * Returns start of unprotected cache block
 *
 */
static void * get_unprotected_cache_block() {
  if (inode_cache_is_enabled) {
    inode_cache_line *start = (inode_cache_line *)cacheMemPtr;
    return &start[INODE_CACHE_LINES];
  } else {
      return cacheMemPtr;
  }
}

static void * get_end_of_cache_block() {
  return ((unsigned char *)cacheMemPtr + (cacheMemSizeInKB * BYTES_IN_KB)); 
}

/*
 *
 * Gets max cache lines in cache
 *
 */
static int max_cache_line() {
  if (!(inode_cache_is_enabled))
      return ((cacheMemSizeInKB * BYTES_IN_KB) / sizeof(cache_line));
  else {
      void* start = get_unprotected_cache_block(); 
      void* end = get_end_of_cache_block();
      return ((char *)end - (char *)start) / sizeof(cache_line);
  }
}

static cache_line *get_cache_line_for_sector(int sectornum) {
    int range = max_cache_line(); 
    /* Because sector 0 will never be cached */
    const unsigned long MULTIPLIER = 2630849305L; // magic prime number
    const unsigned long hash = (sectornum - 1) * 2630849305L;
    int index = hash % range;
    cache_line *start = (cache_line *)get_unprotected_cache_block();
    return &start[index];
}

static int fetch_sector_in_inode_cache(int sectornum, void *buf);
int fetch_sector_in_cache(int sectornum, void *buf) {
    if (cache_is_allocated) {
        cache_line * cached_sector = get_cache_line_for_sector(sectornum);
        if ((cached_sector->sector == sectornum)) {
            memcpy(buf, (const void *)&cached_sector->buf, DISKIMG_SECTOR_SIZE);
            return DISKIMG_SECTOR_SIZE;
        }
        return fetch_sector_in_inode_cache(sectornum, buf);
    }
    return CACHE_ERROR;
}

void save_sector_in_cache(int sectornum, void *buf) {
    if (cache_is_allocated) {
        cache_line* cached_sector = get_cache_line_for_sector(sectornum);
        cached_sector->sector = sectornum;
        memcpy(&cached_sector->buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
    }
}

static int fetch_sector_in_inode_cache(int sectornum, void *buf) {
  if ((cache_is_allocated) && (inode_cache_is_enabled)) {
    inode_cache_line *start = (inode_cache_line *)cacheMemPtr;
    for (int index = 0; index < INODE_CACHE_LINES; index++) {
        if (start[index].sector == sectornum) {
            memcpy(buf, (const void *)&start[index].buf, DISKIMG_SECTOR_SIZE);
            return DISKIMG_SECTOR_SIZE;
        }
    }
  }
  return CACHE_ERROR;
}

/*
 *
 * Saves file sector indirection blocks in cache
 *
 */
int save_file_sector_in_inode_cache(int sectornum, void *buf, void *inp, int typeandindirection) {
  inode_cache_line *start = (inode_cache_line *)cacheMemPtr;

  /* If empty cache line found use it */
  for (int index = 0; index < INODE_CACHE_LINES; index++) {
      if (start[index].inp == NULL) {
          memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
          start[index].typeandindirection = typeandindirection;
          start[index].sector = sectornum;
          start[index].inp = inp;
          return 1;
      }

      /* If stale file's cache line is found use it */
      if (!is_directory(start[index].typeandindirection) && (start[index].inp != inp)) {
          memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
          start[index].typeandindirection = typeandindirection;
          start[index].sector = sectornum;
          start[index].inp = inp;
          return 1;
      }
  }

  for (int index = 0; index < INODE_CACHE_LINES; index++) {
      if (is_singly_indirected(start[index].typeandindirection)) {
          save_sector_in_cache(start[index].sector, &start[index].buf);
          memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
          start[index].typeandindirection = typeandindirection;
          start[index].sector = sectornum;
          start[index].inp = inp;
          return 1;
      }
  }
  return -1;  
}

/*
 *
 * Refer readme.txt for replacement policy
 *
 *
 */
int save_directory_sector_in_inode_cache(int sectornum, void *buf, void *inp, int typeandindirection) {
    inode_cache_line *start = (inode_cache_line *)cacheMemPtr;
    for (int index = 0; index < INODE_CACHE_LINES; index++) {
        if ((start[index].inp == inp) && (is_singly_indirected(start[index].typeandindirection))) {
            memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
            start[index].typeandindirection = typeandindirection;
            start[index].sector = sectornum;
            start[index].inp = inp;
            return 1;
        }

    }

    for (int index = 0; index < INODE_CACHE_LINES; index++) {
        if (start[index].inp == NULL) {
            memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
            start[index].typeandindirection = typeandindirection;
            start[index].sector = sectornum;
            start[index].inp = inp;
            return 1;
        }

        if (!is_directory(start[index].typeandindirection)) {
            memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
            start[index].typeandindirection = typeandindirection;
            start[index].sector = sectornum;
            start[index].inp = inp;
            return 1;
        }
    }

    for (int index = 0; index < INODE_CACHE_LINES; index++) {
        if (is_singly_indirected(start[index].typeandindirection)) {
            save_sector_in_cache(start[index].sector, &start[index].buf);
            memcpy(&start[index].buf, (const void *)buf, DISKIMG_SECTOR_SIZE);
            start[index].typeandindirection = typeandindirection;
            start[index].sector = sectornum;
            start[index].inp = inp;
            return 1;
        }
    }
    return -1;
}


/*
 *
 * Refer README.txt
 *
 */

void save_sector_in_inode_cache(int sectornum, void *buf, void *inp, int typeandindirection) {
  if ((cache_is_allocated) && (inode_cache_is_enabled)) {
    int ret;
    if (is_directory(typeandindirection))
      ret = save_directory_sector_in_inode_cache(sectornum, buf, inp, typeandindirection); 
    else
      ret = save_file_sector_in_inode_cache(sectornum, buf, inp, typeandindirection); 

    /* No space found in inode cache, better save in unprotected cache atleast */
    if (ret < 0) 
      save_sector_in_cache(sectornum, buf);

  } else if ((cache_is_allocated)) {
    save_sector_in_cache(sectornum, buf);
  }
}

/*
 *
 * Erases inode related caches
 *
 */
void erase_sector_in_inode_cache(void *inp) {
  if ((cache_is_allocated) && (inode_cache_is_enabled)) {
    inode_cache_line *start = (inode_cache_line *)cacheMemPtr;
    for (int index = 0; index < INODE_CACHE_LINES; index++) {
      if (start[index].inp == inp) {
        start[index].inp = NULL;
      }
    }
  }
}




