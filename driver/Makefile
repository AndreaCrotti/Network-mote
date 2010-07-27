BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf
SHARED = ../shared

# using c99 C standard, mainly for "for (int i = 0)..."
CC = gcc
STD = -std=c99

PACKET_TYPE = -DNO_IPV6=1
INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF) -I$(SHARED) -I.
LOW6PAN_CARRIED=102
CFLAGS = -D_GNU_SOURCE -DPC -DTOSH_DATA_LENGTH=$(LOW6PAN_CARRIED) -DCLIENT -DDEBUG $(PACKET_TYPE)
WARN = -Wall -Wextra
DEBUG = -ggdb -O0 -pg
FLAGS = $(WARN) $(INCLUDE) $(CFLAGS) $(DEBUG) $(STD)

HEADERS = util.h motecomm.sizes.h config.h hostname.h
TARGETS := gateway client
MAINS := $(addsuffix %.c,$(TARGETS))
CODEFILES := $(filter-out %.test.c,$(wildcard *.c))
OBJECTS := $(patsubst %.c,%.o,$(CODEFILES))
OBJECTS_NOMAIN := $(filter-out $(addsuffix %.o,$(TARGETS)),$(OBJECTS))
TEST_TARGETS := $(patsubst tests/%.test.c,tests/%,$(wildcard tests/*.test.c))
NICETT := $(notdir $(TEST_TARGETS))
IMPLIED_HEADERS := $(patsubst %.c,%.h,$(CODEFILES))
NOPS :=

#For external libraries
EXTERNALS := $(SF)/sfsource.o $(SF)/serialsource.o /usr/lib/libz.so
EXTRAPHONIES = 


#TIME := time -f '\t%E' --
TIME := 

#### RULES ####

all: autogen $(TARGETS) $(TEST_TARGETS) $(NOPS)

$(TARGETS): %: %.o $(OBJECTS_NOMAIN) $(HEADERS)
	@echo "Linking executable file <$@>."
	@$(TIME) $(CC) $(FLAGS) $(OBJECTS_NOMAIN) $(EXTERNALS) $< -o $@

$(OBJECTS): %.o: %.c %.h $(HEADERS)
	@echo    "Compiling object file <$@>."
	@$(TIME) $(CC) $(FLAGS) -c $< -o $@

$(TEST_TARGETS): tests/%: tests/%.test.c $(OBJECTS_NOMAIN)
	@echo "Linking test executable file <$@>."
	@$(TIME) $(CC) $(FLAGS) $(OBJECTS_NOMAIN) $(EXTERNALS) $< -o $@

$(NICETT): %: tests/%

$(IMPLIED_HEADERS): %.h:
	@[ -e $@ ] || touch $@

$(NOPS): %.o: %.c %.h
	@echo "WARNING: NOT MAKING TARGET '$@'!" && echo "         file(s) '$?' was(were) modified but '$@' is in NOPS:" && echo "         $(NOPS)" && touch $@


#### OTHER ####

hostname.h: hostname.sh
	@echo "Creating <hostname.h>."
	@bash hostname.sh


#### DEBUG ####

# whatever changes we have to recompile the tests
reconstruct: reconstruct.c
	$(CC) $(WARN) $(STD) $(INCLUDE) $(CFLAGS) $(DEBUG) -DSTANDALONE -DDEBUG -o reconstruct reconstruct.c

split: split_reconstruct.c
	$(CC) chunker.o reconstruct.o $(STD) $(CFLAGS) $(INCLUDE) $(WARN) $(DEBUG) -DDEBUG -DSTANDALONE -o split_rec split_reconstruct.c


tuntest.dbg: $(OBJECTS) client.c
	$(CC) $(OBJECTS) $(DEBUG) $(EXTERNALS) -o tuntest.dbg

tuntest.pre: $(OBJECTS) client.c
	$(CC) $(OBJECTS) client.c $(CFLAGS) -E -o tuntest.pre

motecomm.pre: motecomm.c motecomm.h motecomm.sizes.h
	$(CC) $(FLAGS) -c motecomm.c -E -o motecomm.pre


#### PHONIES ####
clean:
	@echo "Cleaning."
	@rm *.o hostname.h $(TARGETS) $(TEST_TARGETS) > /dev/null 2>&1; true

tests: $(TEST_TARGETS)
	@$(patsubst %, LAST_TEST='%' && echo "Running test file <$$LAST_TEST>." && ./$$LAST_TEST &&, $(TEST_TARGETS)) true


diag:
	@echo -e " TOSROOT: $(TOSROOT)\n INCLUDE: $(INCLUDE)\n OBJECTS: $(OBJECTS)\n OBJECTS_NOMAIN: $(OBJECTS_NOMAIN)\n CODEFILES: $(CODEFILES)"

run:
	@motelist -c | sed 's/^[^,]*,\([^,]*\),.*/echo Using mote at \1 \&\& .\/client \1/' | head -n 1 | bash

run-w:
	@motelist -c | sed 's/^[^,]*,\([^,]*\),.*/echo Using mote at \1 \&\& .\/gateway \1 eth0/' | head -n 1 | bash

ping:
	@if [ $$(id -u) = 0 ]; then ping -fbi 0.1 10.0.0.2; else echo "You are not root, so I will ping slowly with 1."; ping -fbi 1 10.0.0.2; fi

autogen: hostname.h

force: clean all

.PHONY: clean diag force autogen run ping test tests $(EXTRAPHONIES) $(NICETT)
