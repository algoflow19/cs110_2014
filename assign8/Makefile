# CS110 C++ MapReduce Implementation Example

CXX = g++

CPPFLAGS = -g -Wall -Wno-sign-compare -Wno-vla -pedantic -O0 -std=c++0x -D_GLIBCXX_USE_NANOSLEEP -D_GLIBCXX_USE_SCHED_YIELD -I/usr/class/cs110/local/include/ -I/usr/class/cs110/include

LDFLAGS = -lm -lpthread -L/usr/class/cs110/local/lib -lkey -lthreadpool -lrand -lthreads -L/usr/class/cs110/lib/socket++ -lsocket++ -Wl,-rpath=/usr/class/cs110/lib/socket++ -lssl -lcrypto

# In this section, you list the files that are part of the project.
# If you add/change names of header/source files, here is where you
# edit the Makefile.
PROGRAMS = farmer.cc
EXTRAS = client-socket.cc server-socket.cc block.cc messages.cc txn.cc endpoint.cc utils.cc
HEADERS = $(EXTRAS:.cc=.h)
SOURCES = $(PROGRAMS) $(EXTRAS)
OBJECTS = $(SOURCES:.cc=.o)
TARGETS = $(PROGRAMS:.cc=)

default: $(TARGETS)

farmer: farmer.o client-socket.o server-socket.o block.o messages.o txn.o endpoint.o utils.o
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
.PHONY: test clean

test:
	./reset.sh
	./start.sh

clean:
	@rm -f $(TARGETS) $(OBJECTS) Makefile.dependencies
