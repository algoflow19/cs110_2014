# CS110 Exceptional Control Flow Examples

CC = gcc

# The CFLAGS variable sets compile flags for g:
#  -g          compile with debug information
#  -Wall       give all diagnostic warnings
#  -pedantic   require compliance with ANSI standard
#  -O0         do not optimize generated code
CFLAGS = -g -Wall -pedantic -O0 -std=gnu99 -I/usr/class/cs110/local/include

# The LDFLAGS variable sets flags for linker
#  -lm       link in libm (math library)
LDFLAGS =

# In this section, you list the files that are part of the project.
# If you add/change names of header/source files, here is where you edit the Makefile.
HEADERS = 
SOURCES = basic-fork.c fork-puzzle.c slave-master.c separate.c \
          reap-as-they-exit.c reap-in-fork-order.c \
	  mysystem.c segfault.c simplesh.c loop.c pipe-experiment.c subprocess.c \
          signal-experiment.c independent-children.c close-children.c close-children-improved.c \
	  job-list-synchronization.c job-list-synchronization-improved.c \
          kill-puzzle.c
OBJECTS = $(SOURCES:.c=.o)
TARGETS = $(SOURCES:.c=)

default: $(TARGETS)

basic-fork: basic-fork.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

fork-puzzle: fork-puzzle.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

slave-master: slave-master.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

separate: separate.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

reap-as-they-exit: reap-as-they-exit.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

reap-in-fork-order: reap-in-fork-order.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

mysystem: mysystem.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

segfault: segfault.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

simplesh: simplesh.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

loop: loop.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

pipe-experiment: pipe-experiment.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

subprocess: subprocess.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

signal-experiment: signal-experiment.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

independent-children: independent-children.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

close-children: close-children.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

close-children-improved: close-children-improved.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

job-list-synchronization: job-list-synchronization.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

job-list-synchronization-improved: job-list-synchronization-improved.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

kill-puzzle: kill-puzzle.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# In make's default rules, a .o automatically depends on its .cc file
# (so editing the .cc will cause recompilation into its .o file).
# The line below creates additional dependencies, most notably that it
# will cause the .cc to recompiled if any included .h file changes.

Makefile.dependencies:: $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -MM $(SOURCES) > Makefile.dependencies

-include Makefile.dependencies

# Phony means not a "real" target, it doesn't build anything
# The phony target "clean" is used to remove all compiled object files.
# The phony target "spartan" is used to remove all compilation products and extra backup files. 

.PHONY: clean spartan

clean:
	@rm -f $(TARGETS) $(OBJECTS) core Makefile.dependencies

spartan: clean
	@rm -f *~
