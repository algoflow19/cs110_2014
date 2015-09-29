#ifndef _PATHSTORE_H_
#define _PATHSTORE_H_
#include "assign1/inode.h"

typedef struct Pathstore {
  struct PathstoreElement *elementList;
  void                    *fshandle;
} Pathstore;

Pathstore* Pathstore_create(void *fshandle);
void       Pathstore_destory(Pathstore *store);
/* 
 * Optimization gets inumber as argument so that we could
 * utilize checksum by inode to weed out duplicates
 *
 */
char*      Pathstore_path(Pathstore *store, char *pathname,
                          int discardDuplicateFiles, struct inode *inp, int inode_iget_ret);

void Pathstore_Dumpstats(FILE *file);

#endif // _PATHSTORE_H_
