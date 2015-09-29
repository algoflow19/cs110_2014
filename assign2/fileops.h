#ifndef _FILEOPS_H_
#define _FILEOPS_H_
#include <stdio.h>
#include "assign1/inode.h"
#include "assign1/chksumfile.h"
#include "assign1/file.h"

void *Fileops_init(char *diskpath);
int Fileops_open(char *pathname);
int Fileops_read(int fd, char *buffer, int length);
int Fileops_getchar(int fd);
int Fileops_tell(int fd);
int Fileops_close(int fd);
int Fileops_isfile(char *pathname);
void Fileops_Dumpstats(FILE *file);
int optimized_Fileops_isfile(int inumber, struct inode *inp, int *inode_iget_ret);
int optimized_Fileops_open(char *pathname, int inumber, struct inode *inp, int inode_iget_ret);

#endif // _FILEOPS_H_
