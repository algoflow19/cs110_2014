# CS110 C++ MapReduce Implementation Example

CXX = g++

# The CPPFLAGS variable sets compile flags for g++:
#  -g          compile with debug information
#  -Wall       give all diagnostic warnings
#  -pedantic   require compliance with ANSI standard
#  -O0         do not optimize generated code
#  -std=c++0x  go with the c++0x experimental extensions for thread support (and other nifty things)
#  -D_GLIBCXX_USE_NANOSLEEP included for this_thread::sleep_for and this_thread::sleep_until support
#  -D_GLIBCXX_USE_SCHED_YIELD included for this_thread::yield support
CPPFLAGS = -g -Wall -pedantic -O0 -std=c++0x -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD -I/usr/class/cs110/local/include/ -I/usr/class/cs110/include

# The LDFLAGS variable sets flags for linker
# -lm        link to libm (math library)
# -pthread   link in libpthread (thread library) to back C++11 extensions
# -lthreads  link to course-specific concurrency functions and classes
# -lrand     link to a C++11-backed random number generator module
# -socket++  link to third party socket++ library for sockbuf and iosockstream classes
LDFLAGS = -lm -lpthread -L/usr/class/cs110/local/lib -lthreadpool -L/usr/class/cs110/local/lib -lthreads -L/usr/class/cs110/local/lib -lrand -L/usr/class/cs110/lib/socket++ -lsocket++ -Wl,-rpath=/usr/class/cs110/lib/socket++

# In this section, you list the files that are part of the project.
# If you add/change names of header/source files, here is where you
# edit the Makefile.
PROGRAMS = mr.cc mrw.cc word-count-mapper.cc word-count-reducer.cc
EXTRAS = mapreduce-server.cc mapreduce-worker.cc client-socket.cc server-socket.cc mr-nodes.cc mr-messages.cc mr-env.cc mr-random-utils.cc
HEADERS = $(EXTRAS:.cc=.h) mapreduce-server-exception.h thread-pool.h
SOURCES = $(PROGRAMS) $(EXTRAS)
OBJECTS = $(SOURCES:.cc=.o)
TARGETS = $(PROGRAMS:.cc=)

default: $(TARGETS)

directories:
	rm -fr files/map-output
	rm -fr files/reduce-input
	rm -fr files/output
	mkdir -p files/map-output
	mkdir -p files/reduce-input
	mkdir -p files/output

mr: mr.o mapreduce-server.o server-socket.o client-socket.o mr-nodes.o mr-messages.o mr-env.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

mrw: mrw.o mapreduce-worker.o client-socket.o mr-messages.o mr-env.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)
	ln -fs ../../$@ files/bin/

word-count-mapper: word-count-mapper.o mr-random-utils.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)
	ln -fs ../../$@ files/bin/

word-count-reducer: word-count-reducer.o mr-random-utils.o
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)
	ln -fs ../../$@ files/bin/

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

filefree:
	@rm -fr files/map-output/* files/reduce-input/* files/output/*	

clean: filefree
	@rm -f $(TARGETS) $(OBJECTS) mr core Makefile.dependencies files/bin/*

spartan: clean
	@rm -f *~
