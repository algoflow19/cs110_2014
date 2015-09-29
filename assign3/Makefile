# Makefile for Assignment 3

DRIVER = ./sdriver.pl
TSHARGS = "-p"
CC = gcc

# The CFLAGS variable sets compile flags for g: 
#  -g          compile with debug information
#  -Wall       give all diagnostic warnings
#  -pedantic   require compliance with ANSI standard
#  -O0         do not optimize generated code 
CFLAGS = -g -Wall -pedantic -O0 -std=gnu99 -I/usr/class/cs110/local/include

TARGETS = tsh tsh-parse-test myspin mysplit mystop myint mycat

all: $(TARGETS)

tsh: tsh.o tsh-parse.o tsh-jobs.o tsh-state.o tsh-signal.o
	gcc -o tsh tsh.o tsh-parse.o tsh-jobs.o tsh-state.o tsh-signal.o

tsh-parse-test: tsh-parse-test.o tsh-parse.o
	gcc -o tsh-parse-test tsh-parse-test.o tsh-parse.o

# 
# Regression tests
#
tests: 	tsh test01 test02 test03 test04 test05 test06 test07 test08 test09 test10\
	test11 test12 test13 test14 test15 test16 test17 test18 test19
test01:
	$(DRIVER) -t traces/trace01.txt -s ./tsh -a $(TSHARGS)
test02:
	$(DRIVER) -t traces/trace02.txt -s ./tsh -a $(TSHARGS)
test03:
	$(DRIVER) -t traces/trace03.txt -s ./tsh -a $(TSHARGS)
test04:
	$(DRIVER) -t traces/trace04.txt -s ./tsh -a $(TSHARGS)
test05:
	$(DRIVER) -t traces/trace05.txt -s ./tsh -a $(TSHARGS)
test06:
	$(DRIVER) -t traces/trace06.txt -s ./tsh -a $(TSHARGS)
test07:
	$(DRIVER) -t traces/trace07.txt -s ./tsh -a $(TSHARGS)
test08:
	$(DRIVER) -t traces/trace08.txt -s ./tsh -a $(TSHARGS)
test09:
	$(DRIVER) -t traces/trace09.txt -s ./tsh -a $(TSHARGS)
test10:
	$(DRIVER) -t traces/trace10.txt -s ./tsh -a $(TSHARGS)
test11:
	$(DRIVER) -t traces/trace11.txt -s ./tsh -a $(TSHARGS)
test12:
	$(DRIVER) -t traces/trace12.txt -s ./tsh -a $(TSHARGS)
test13:
	$(DRIVER) -t traces/trace13.txt -s ./tsh -a $(TSHARGS)
test14:
	$(DRIVER) -t traces/trace14.txt -s ./tsh -a $(TSHARGS)
test15:
	$(DRIVER) -t traces/trace15.txt -s ./tsh -a $(TSHARGS)
test16:
	$(DRIVER) -t traces/trace16.txt -s ./tsh -a $(TSHARGS)
test17:
	$(DRIVER) -t traces/trace17.txt -s ./tsh -a $(TSHARGS)
test18:
	$(DRIVER) -t traces/trace18.txt -s ./tsh -a $(TSHARGS)
test19:
	$(DRIVER) -t traces/trace19.txt -s ./tsh -a $(TSHARGS)

# clean up
clean:
	rm -f $(TARGETS) *.o *~

spartan: clean
	rm -fr *~
