/**
 * File: filedump.c
 * ----------------
 * Program designed to emaulte a bare-bone versions of cat (which is actually /bin/cat
 * on the myths).  Our version reads in the contents of a named file and publishes its
 * contents to standard output.  If the user doesn't specify a file name (e.g. they type in
 * filedump and nothing else), then the application simply echoes whatever the user types
 * in to standard output instead.
 *
 * While there are a jillion ways to implement this, one approach--a particularly 
 * elegant one, imo--is to associate file descriptor 0 (aka STDIN_FILENO) with the 
 * specified file if it's provided.  By doing so, we can always draw character
 * content from STDIN_FILENO once we arrive at the point where we need to start calling
 * the read system call.
 */

#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
  // error checking omitted
  if (argc == 2) {
    int fd = open(argv[1], O_RDONLY);
    dup2(fd, STDIN_FILENO);
    close(fd);
  }
  
  char buffer[1024];
  while (true) {
    size_t numBytesRead = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (numBytesRead == 0) break;
    size_t numBytesWritten = 0;
    while (numBytesWritten < numBytesRead) {
      numBytesWritten +=
        write(STDOUT_FILENO, buffer + numBytesWritten, 
              numBytesRead - numBytesWritten);
    }
  }
  
  return 0;
}
