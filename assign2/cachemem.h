#ifndef _CACHEMEM_H_
#define _CACHEMEM_H_
#include "assign1/inode.h"
/**
 * The main export of the cachemem module is the memory for the cache
 * pointed to by the following global variables:
 *
 * cacheMemSizeInKB - The size of the cache memory in kiloytes. 
 * cacheMemPtr      - Starting address of the cache memory. 
 */

extern int cacheMemSizeInKB;
extern void *cacheMemPtr;

#define CACHEMEM_MAX_SIZE (64*1024*1024)
#define CACHE_ERROR -1

int CacheMem_Init(int sizeInKB);
void save_sector_in_cache(int sectornum, void *buf);
int fetch_sector_in_cache(int sectornum, void *buf);
void save_sector_in_inode_cache(int sectornum, void *buf, void *inp, int indirection);
void erase_sector_in_inode_cache(void *inp);

#endif // _CACHEMEM_H_
