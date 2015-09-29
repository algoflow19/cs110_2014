#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <getopt.h>
#include <string.h>

#include <sys/time.h>
#include <sys/resource.h>

#include "index.h"
#include "debug.h"
#include "pathstore.h"
#include "fileops.h"
#include "diskimg.h"
#include "disksim.h"
#include "scan.h"
#include "cachemem.h"

static void PrintUsageAndExit(char *progname);
static void DumpStats(FILE *file);
static void DumpUsageStats(FILE *file);
static int  QueryWord(char *word, Index *ind, FILE *file );

static void BuildDiskIndex(char *diskpath);
static void TestServiceBySingleWord(char *queryWord);
static void TestServiceByFileOfWords(char *queryFile);

/*
 * Gloabl program options
 */
int quietFlag = 0;
int diskLatency = 8000;
int diskBusyWaitEnable = 0;

#define INFINITE_CACHE_SIZE (64*1024*1024)  /* 64MB is infinite */

/*
 * The index store
 */
static Index *diskIndex = NULL;
static Pathstore *store = NULL;

int main(int argc, char *argv[]) {
  int opt;
  char *queryWord = NULL;
  char *queryFile = NULL;
  int cacheSizeInKB = 0;

  while ((opt = getopt(argc, argv, "ql:d:w:f:bc:")) != -1) {
    switch (opt) {
      case 'q':
        quietFlag = 1;
        break;
      case 'l':
        diskLatency = atoi(optarg);
        break;
      case 'c':
        cacheSizeInKB = atoi(optarg);
        break;
      case 'b':
        diskBusyWaitEnable = 1;
        break;
      case 'w':
        queryWord = strdup(optarg);
        break;
      case 'f':
        queryFile = strdup(optarg);
        break;
      case 'd': {
        char *c = optarg;
        while (*c) {
          Debug_SetFlag(*c, 1);
          c++;
        }
        break;
      }
      default:
        PrintUsageAndExit(argv[0]);
    }
  }
  if (optind != argc-1) {
    PrintUsageAndExit(argv[0]);
  }

  /*
   * Allocate Memory for any caching of data. 0 means infinite cache.
   */
  if (cacheSizeInKB == 0) {
    cacheSizeInKB = INFINITE_CACHE_SIZE/1024;  
  }
  int retVal = CacheMem_Init(cacheSizeInKB);
  if (retVal < 0) {
    fprintf(stderr, "Can't allocate memory for cache\n");
    return 1;
  }

  char *diskpath = diskpath = argv[optind];
  BuildDiskIndex(diskpath);

  if (queryWord) {
    TestServiceBySingleWord(queryWord);
  }
  if (queryFile) {
    TestServiceByFileOfWords(queryFile);
  }

  if (!quietFlag) {
    printf("************ Stats ***************\n");
    DumpStats(stdout);
    DumpUsageStats(stdout);
  }

  exit(EXIT_SUCCESS);
  return 0;
}

void BuildDiskIndex(char *diskpath) {
  void *fshandle = Fileops_init(diskpath);
  if (fshandle == NULL) {
    fprintf(stderr, "Error initializing  %s\n", diskpath);
    exit(EXIT_FAILURE);
  }

  store = Pathstore_create(fshandle);
  if (store == NULL) {
    fprintf(stderr, "Can't create pathstore\n");
    exit(EXIT_FAILURE);
  }

  diskIndex = Index_Create();
  if (diskIndex == NULL) {
    fprintf(stderr, "Can't create index\n");
    exit(EXIT_FAILURE);
  }
  if (!quietFlag) {
    printf("Starting index build ....");
  }

  int64_t startTime = Debug_GetTimeInMicrosecs();
  int err = Scan_TreeAndIndex("/", diskIndex, store, /* discardDups = */ 1);
  if (err) {
    fprintf(stderr, "Error creating index\n");
    exit(EXIT_FAILURE);
  }
  int64_t endTime = Debug_GetTimeInMicrosecs();

  if (!quietFlag) {
    printf("\nIndex disk %s (latency %d) completed in %f seconds\n",
           diskpath, diskLatency, (endTime - startTime)/1000000.0);
  }
}

void TestServiceBySingleWord(char *queryWord) {
  QueryWord(queryWord, diskIndex, stdout);

  /* Do timing without printf */
  int64_t startTime = Debug_GetTimeInMicrosecs();
  QueryWord(queryWord, diskIndex, NULL);
  int64_t endTime = Debug_GetTimeInMicrosecs();

  if (!quietFlag) {
    printf("QueryWord \'%s\' took %f microseconds\n",queryWord,
	   (double)(endTime - startTime));
  }
}

void TestServiceByFileOfWords(char *queryFile) {
  const int maxWordSize = 64 + 1;  // A word is 64 characters plus a null byte
  FILE *file = fopen(queryFile, "r");

  if (file == NULL) {
    perror("fopen");
    fprintf(stderr, "Can't open query file %s\n",queryFile);
    exit(EXIT_FAILURE);
  }

  char line[maxWordSize];
  while (fgets(line, maxWordSize, file) != NULL) {
    int len = strlen(line);
    if (len > 0) {
	  /* Strip the \n character */
	  if (line[len-1] == '\n') line[len-1] = 0;
	  QueryWord(line,diskIndex, stdout);
    }
  }
  rewind(file);

  /* Do timing without printf */
  int64_t startTime = Debug_GetTimeInMicrosecs();
  while (fgets(line, maxWordSize, file) != NULL) {
    QueryWord(line,diskIndex, NULL);
  }
  int64_t endTime = Debug_GetTimeInMicrosecs();

  if (!quietFlag) {
    printf("QueryFile %s took %f microseconds\n",
           queryFile, (double)(endTime - startTime));
  }

  fclose(file);
}

int QueryWord(char *word, Index *ind, FILE *file) {
  IndexLocationList *loc = Index_RetrieveEntry(ind,word);
  if (loc == NULL) {
    if (file)
      fprintf(file, "Word %s not found\n", word);
    return 0;
  }

  while (loc) {
    if (file)
      fprintf(file,"Word %s @ %s:%d\n", word, loc->item.pathname, loc->item.offset);
    loc = loc->nextLocation;
  }

  return 1;
}

void DumpStats(FILE *file) {
  disksim_dumpstats(file);
  diskimg_dumpstats(file);
  Scan_Dumpstats(file);
  Index_Dumpstats(file);
  Pathstore_Dumpstats(file);
  Fileops_Dumpstats(file);
}

void DumpUsageStats(FILE *file) {
  struct rusage usage;
  int err = getrusage(RUSAGE_SELF, &usage);
  if (err) {
    fprintf(stderr, "Error getting resource usage\n");
    return;
  }

  fprintf(file, "Usage: %f usertime, %f systemtime, "
          "%ld voluntary ctxt switches, %ld involuntary ctxt switches\n",
	  usage.ru_utime.tv_sec + usage.ru_utime.tv_usec/1000000.0,
	  usage.ru_stime.tv_sec + usage.ru_stime.tv_usec/1000000.0,
	  usage.ru_nvcsw,  usage.ru_nivcsw);
}

void PrintUsageAndExit(char *progname) {
  fprintf(stderr, "Usage: %s <options> diskimagePath\n", progname);
  fprintf(stderr, "where <options> can be:\n");
  fprintf(stderr, "-q     don't print extra info\n");
  fprintf(stderr, "-l N   set simulated disk latency to N microseconds\n");
  fprintf(stderr, "-b     simulate disk latency by busy-waiting\n");
  fprintf(stderr, "-w W   query index for word W\n");
  fprintf(stderr, "-f F   read query words from file F\n");
  fprintf(stderr, "-d debugFlags   set the debug files in the debugFlags string\n");
  exit(EXIT_FAILURE);
}

