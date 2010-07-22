BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf
SHARED = ../shared

CC = gcc
STD = -std=c99

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF) -I$(SHARED)
# this changes the size of the message (found http://www.tinyos.net/tinyos-2.x/doc/html/tep111.html)
LOW6PAN_CARRIED=102
CFLAGS = -D_GNU_SOURCE -DPC -DTOSH_DATA_LENGTH=$(LOW6PAN_CARRIED)
WARN = -Wall -Wextra
DEBUG = -ggdb -O0 -pg
FLAGS = $(WARN) $(INCLUDE) $(CFLAGS) $(DEBUG) $(STD)

HEADERS = util.h motecomm.sizes.h config.h hostname.h
OBJECTS := $(patsubst %.c, %.o, $(wildcard *.c))
NOPS =
#For external libraries
EXTERNALS = $(SF)/sfsource.o $(SF)/serialsource.o
SRC = client.c
EXTRAPHONIES = 

#### RULES ####

all: autogen tuntest $(NOPS)

tuntest: $(OBJECTS)
	@echo "Linking <$@>."
	@$(CC) $(FLAGS) $(OBJECTS) $(EXTERNALS) -o tuntest

$(OBJECTS): %.o: %.c %.h $(HEADERS)
	@echo "Compiling <$@>."
	@$(CC) $(FLAGS) -c $< -o $@

$(NOPS): %.o: %.c %.h
	@echo "WARNING: NOT MAKING TARGET '$@'!" && echo "         file(s) '$?' was(were) modified but '$@' is in NOPS:" && echo "         $(NOPS)" && touch $@


#### OTHER ####

hostname.h: hostname.sh
	@echo "Creating <hostname.h>."
	@bash hostname.sh


#### DEBUG ####

reconstruct: reconstruct.c
	$(CC) $(WARN) $(STD) $(INCLUDE) $(CFLAGS) $(DEBUG) -DSTANDALONE -DDEBUG -o reconstruct reconstruct.c

variables.o: variables.c

split_rec: test_split_reconstruct.c variables.o
	$(CC) variables.o chunker.o reconstruct.o $(STD) $(CFLAGS) $(INCLUDE) $(WARN) $(DEBUG) -DDEBUG -DTEST -o split_rec test_split_reconstruct.c

tuntest.dbg: $(OBJECTS) client.c
	$(CC) $(OBJECTS) $(DEBUG) $(EXTERNALS) -o tuntest.dbg

tuntest.pre: $(OBJECTS) client.c
	$(CC) $(OBJECTS) client.c $(CFLAGS) -E -o tuntest.pre

motecomm.pre: motecomm.c motecomm.h motecomm.sizes.h
	$(CC) $(FLAGS) -c motecomm.c -E -o motecomm.pre

motecomm: motecomm.c motecomm.h motecomm.sizes.h
	$(CC) $(FLAGS) -DSTANDALONE motecomm.c util.o $(EXTERNALS) -o motecomm


#### PHONIES ####
clean:
	@echo "Cleaning."
	@rm *.o hostname.h tuntest > /dev/null 2>&1

diag:
	@echo TOSROOT: $(TOSROOT) INCLUDE: $(INCLUDE)

run:
	@motelist -c | sed 's/^[^,]*,\([^,]*\),.*/.\/tuntest \1/' | bash

ping:
	ping -fbi 0.1 10.0.0.2

autogen: hostname.h

force: clean tuntest

.PHONY: clean diag force autogen run ping $(EXTRAPHONIES)
