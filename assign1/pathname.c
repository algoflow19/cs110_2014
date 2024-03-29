
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

/*
 * Macros
 */
// Max path length
#define MAXPATH 1024

/*
 * Helper Functions
 */
static inline int validate_pathname_lookup(struct unixfilesystem *fs, const char * pathname);

/*
 * Function : validate_pathname_lookup
 * Usage : if ((validate_pathname_lookup(fs, pathname) == SUCCESS))
 * --------------------------------------------------------------------
 * This function validates the inputs for pathname_lookup and returns 0/SUCCESS
 * if they are valid else -1/FAILURE
 */
static inline int validate_pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if ((fs) && (pathname) && (pathname[0] == '/') && (strlen(pathname) < MAXPATH))
        return SUCCESS;
    return FAILURE;
}

/*
 * Function : pathname_lookup
 * Usage : int inumber = pathname_lookup(fs, pathname)
 * -------------------------------------------------------
 * This function returns the inode number associated with the specified pathname.
 * Returns FAILURE / -1 if an error is encountered.
 */
int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
    if ((validate_pathname_lookup(fs, pathname) == SUCCESS)) {
        char *path = strdup(pathname);
        // We are starting with an absolute path.
        // The root inumber is well known.
        int inumber = ROOT_INUMBER;
        // Extracts the immediate member of root directory
        // from absolute path
        char *sub_path = strtok(path, "/");
        while (sub_path != NULL) {
            struct direntv6 member;
            // If member exists, extract inumber which will be used 
            // in the next iteration.
            if ((directory_findname(fs, sub_path, inumber, &member)) == SUCCESS)
                inumber = member.d_inumber;
            else {
                inumber = FAILURE;
                break;
            }
            // Extracts the immediate member of the current directory
            // from absolute path.
            sub_path = strtok(NULL, "/");
        }
        if ((path))
            free(path);
        // inumber of the last name is returned
        return inumber;
    } else {
        fprintf(stderr, "pathname_lookup(path=%s fs=0x%p) Validation failed. Returing -1.\n", pathname, fs);
        return FAILURE;
    }
}
