# CS110 C++ Threading and Concurrency Examples

CXX = g++

# The CPPFLAGS variable sets compile flags for g++:
#  -g          compile with debug information
#  -Wall       give all diagnostic warnings
#  -pedantic   require compliance with ANSI standard
#  -O0         do not optimize generated code
#  -std=c++0x  go with the c++0x experimental extensions for thread support (and other nifty things)
#  -D_GLIBCXX_USE_NANOSLEEP included for this_thread::sleep_for and this_thread::sleep_until support
#  -D_GLIBCXX_USE_SCHED_YIELD included for this_thread::yield support
CPPFLAGS = -g -Wall -pedantic -O0 -std=c++0x -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD -I/usr/class/cs110/local/include/

# The LDFLAGS variable sets flags for linker
#  -lm       link in libm (math library)
#  -lpthread link in libpthread (thread library) to back C++11 extensions
LDFLAGS = -lm -lpthread -L/usr/class/cs110/local/lib/ -lthreads -lrand

# In this section, you list the files that are part of the project.
# If you add/change names of header/source files, here is where you
# edit the Makefile.
HEADERS = myth-buster-service.h
SOURCES = introverts.cc greeters.cc tickets.cc \
          dining-philosophers-with-deadlock.cc dining-philosophers-with-busy-waiting.cc \
          dining-philosophers-with-condition-variable.cc dining-philosophers-with-semaphore.cc \
          reader-writer.cc myth-buster-sequential.cc myth-buster-concurrent.cc \
	  ice-cream-store.cc
EXTRAS = myth-buster-service.cc
OBJECTS = $(SOURCES:.cc=.o) $(EXTRAS:.cc=.o)
TARGETS = $(SOURCES:.cc=)

default: $(TARGETS)

introverts: introverts.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

greeters: greeters.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

tickets: tickets.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

dining-philosophers-with-deadlock: dining-philosophers-with-deadlock.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

dining-philosophers-with-busy-waiting: dining-philosophers-with-busy-waiting.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

dining-philosophers-with-condition-variable: dining-philosophers-with-condition-variable.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

dining-philosophers-with-semaphore: dining-philosophers-with-semaphore.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

reader-writer: reader-writer.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

myth-buster-sequential: myth-buster-sequential.o myth-buster-service.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

myth-buster-concurrent: myth-buster-concurrent.o myth-buster-service.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

ice-cream-store: ice-cream-store.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

# In make's default rules, a .o automatically depends on its .cc file
# (so editing the .cc will cause recompilation into its .o file).
# The line below creates additional dependencies, most notably that it
# will cause the .cc to recompiled if any included .h file changes.

Makefile.dependencies:: $(SOURCES) $(HEADERS)
	$(CXX) $(CPPFLAGS) -MM $(SOURCES) > Makefile.dependencies

-include Makefile.dependencies

# Phony means not a "real" target, it doesn't build anything
# The phony target "clean" is used to remove all compiled object files.
# The phony target "spartan" is used to remove all compilation products and extra backup files. 

.PHONY: clean spartan

clean:
	@rm -f $(TARGETS) $(OBJECTS) core Makefile.dependencies

spartan: clean
	@rm -f *~
