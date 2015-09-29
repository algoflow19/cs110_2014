#ifndef _DISKIMG_NEW_H_
#define _DISKIMG_NEW_H_

#include "assign1/diskimg.h"
#include <stdio.h>

void diskimg_dumpstats(FILE *file);
int diskimg_readsector_inode(int fd, int sectorNum, void *buf, void *inp, int indirection);

#endif // _DISKIMG_NEW_H_
