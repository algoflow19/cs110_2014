/*
 * pathstore.c  - Store pathnames for indexing
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

#include "index.h"
#include "fileops.h"
#include "pathstore.h"
#include "assign1/chksumfile.h"

typedef struct PathstoreElement {
  char *pathname;
  /*
   * Optimization to store checksum string
   * of pathstore elements in their structure
   */
  char pathchksumstring[CHKSUMFILE_SIZE];
  struct PathstoreElement *nextElement;
} PathstoreElement;

static uint64_t numdifferentfiles = 0;
static uint64_t numsamefiles = 0;
static uint64_t numdiffchecksum = 0;
static uint64_t numdups = 0;
static uint64_t numcompares = 0;
static uint64_t numstores = 0;

/*
 * Modified to intelligently compare incoming checksum string with already 
 * known files' checksum string
 */
static int SameFileIsInStore(Pathstore *store, char *pathname, char *pathchksumstring);
static int IsSameFile(char *pathname1, char *pathname2, char *pathchksumstring, char *incomingpathchksumstring);

Pathstore* Pathstore_create(void *fshandle) {
  Pathstore *store = malloc(sizeof(Pathstore));
  if (store == NULL)
    return NULL;

  store->elementList = NULL;
  store->fshandle = fshandle;
  return store;
}

/**
 * Free up all the sources allocated for a pathstore.
 */
void Pathstore_destory(Pathstore *store) {
  PathstoreElement *e = store->elementList;

  while (e) {
    PathstoreElement *next = e->nextElement;
    free(e->pathname);
    free(e);
    e = next;
  }
  free(store);
}

static int simplePathnameInStoreCheck(Pathstore *store, char *pathname) {
  PathstoreElement *e = store->elementList;
  int count = 0;
  while (e) {
      count++;
      if (strcmp(pathname, e->pathname) == 0) { // Same pathname must be same file
          return count;
      }
      e = e->nextElement;
  }
  return -1;
}

/**
 * Store a pathname in the pathname store.
 * Optimization : Gets inumber as argument to utilize chksumfile_byinumber
 */
char *Pathstore_path(Pathstore *store, char *pathname, int discardDuplicateFiles, struct inode *inp, int inode_iget_ret) {
  assert(store != NULL);
  assert(pathname != NULL);

  numstores++;
  char pathchksumstring[CHKSUMFILE_SIZE];

  if (discardDuplicateFiles) {

    /*
     * Optimization, do not do checksum by inode operation 
     * without ruling out the pathname.
     */
    int count;
    if ((count = simplePathnameInStoreCheck(store, pathname)) > 0) {
        /* Updating the counter for compares only for positive result */
        numcompares += count;
        numdups++;
        return NULL;
    }

    /*
     * Optimization : Calculate the checksum of the incoming path
     * ahead of calling SameFileIsInStore.
     *
     * The modest ~1MB cache will be littered with file contents which
     * will be easy picking for following scan and index operations.
     *
     * The protected inode cache marked with inode struct pointer will not be flushed.
     *
     */
     if ((optimized_chksumfile_byinode((struct unixfilesystem *) (store->fshandle), pathchksumstring, inp, inode_iget_ret)) < 0)
        memset(pathchksumstring, '\0', CHKSUMFILE_SIZE);
    if (SameFileIsInStore(store, pathname, pathchksumstring)) {
      numdups++;
      return NULL;
    }
  }

  PathstoreElement *e = malloc(sizeof(PathstoreElement));
  if (e == NULL) {
    return NULL;
  }

  e->pathname = strdup(pathname);
  if (e->pathname == NULL) {
    free(e);
    return NULL;
  }

  /*
   * Optimization : Saving memcpy cpu cycles when duplicate files are
   * allowed
   */
  if (discardDuplicateFiles) 
    memcpy(e->pathchksumstring, (const void *)pathchksumstring, CHKSUMFILE_SIZE);
  e->nextElement = store->elementList;
  store->elementList = e;
  return e->pathname;
}

/**
 * Is this file the same as any other one in the store
 * Modified to receving incoming path checksum string.
 */
static int SameFileIsInStore(Pathstore *store, char *pathname, char *pathchksumstring) {
  PathstoreElement *e = store->elementList;
  while (e) {
    if (IsSameFile(pathname, e->pathname, e->pathchksumstring, pathchksumstring)) {
      return 1;  // In store already
    }
    e = e->nextElement;
  }
  return 0; // Not found in store
}

/**
 * Do the two pathnames refer to a file with the same contents.
 * Modified to compare the incoming checkum with already known
 * checksums of known files.
 */
static int IsSameFile(char *pathname1, char *pathname2, char *pathchksumstring, char *incomingpathchksumstring) {
  numcompares++;

  if (chksumfile_compare(pathchksumstring, incomingpathchksumstring) == 0) {
    numdiffchecksum++;
    return 0;  // Checksum mismatch, not the same file
  }

  /*
   *
   * Very very very unlikely to hit the following part of 
   * code. No need of any optimization.
   *
   * Referred : Avalanche Effect, Cryptography
   *
   */

  // Checksums match, do a content comparison
  int fd1 = Fileops_open(pathname1);
  if (fd1 < 0) {
    fprintf(stderr, "Can't open path %s\n", pathname1);
    return 0;
  }

  int fd2 = Fileops_open(pathname2);
  if (fd2 < 0) {
    Fileops_close(fd1);
    fprintf(stderr, "Can't open path %s\n", pathname2);
    return 0;
  }

  int ch1, ch2;
  do {
    ch1 = Fileops_getchar(fd1);
    ch2 = Fileops_getchar(fd2);

    if (ch1 != ch2) {
      break; // Mismatch - exit loop with ch1 != ch2
    }
  } while (ch1 != -1);

  // if files match then ch1 == ch2 == -1
  Fileops_close(fd1);
  Fileops_close(fd2);

  if (ch1 == ch2) {
    numsamefiles++;
  } else {
    numdifferentfiles++;
  }

  return ch1 == ch2;
  return 1;
}

void Pathstore_Dumpstats(FILE *file) {
  fprintf(file,
          "Pathstore:  %"PRIu64" stores, %"PRIu64" duplicates\n"
          "Pathstore2: %"PRIu64" compares, %"PRIu64" checksumdiff, "
          "%"PRIu64" comparesuccess, %"PRIu64" comparefail\n",
          numstores, numdups, numcompares, numdiffchecksum,
          numsamefiles, numdifferentfiles);
}
