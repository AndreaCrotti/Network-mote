BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf

CC = gcc

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF)
# this changes the size of the message (found http://www.tinyos.net/tinyos-2.x/doc/html/tep111.html)
LOW6PAN_CARRIED=102
CFLAGS = -D_GNU_SOURCE -DPC -DTOSH_DATA_LENGTH=$(LOW6PAN_CARRIED)
WARN = -Wall -Wextra
FLAGS = $(WARN) $(INCLUDE) $(CFLAGS) -ggdb -O0

<<<<<<< HEAD
OBJECTS = tunnel.o motecomm.o util.o glue.o chunker.o
          #config.o tun_dev.o routing.o 
#OBJECTS = 
=======
HEADERS = util.h motecomm.sizes.h config.h hostname.h
OBJECTS = tunnel.o motecomm.o util.o glue.o chunker.o
NOPS = tun_dev.o routing.o split.o 
>>>>>>> 1e7e2277b105267124b5f7e870feab30fb6b8a91
#For external libraries
EXTERNALS = $(SF)/sfsource.o $(SF)/serialsource.o
SRC = client.c
EXTRAPHONIES = 


#### RULES ####

all: autogen tuntest $(NOPS)

tuntest: $(OBJECTS) client.c
	$(CC) $(FLAGS) $(OBJECTS) $(EXTERNALS) client.c $(CFLAGS) -o tuntest

$(OBJECTS): %.o: %.c %.h $(HEADERS)
	$(CC) $(FLAGS) -c $< -o $@

<<<<<<< HEAD
routing.o: routing.c
	gcc $(FLAGS) -c routing.c -o routing.o
=======
$(NOPS): %.o: %.c %.h
	echo "WARNING: NOT MAKING TARGET '$@'!" && echo "         file(s) '$?' was(were) modified but '$@' is in NOPS:" && echo "         $(NOPS)" && touch $@

>>>>>>> 1e7e2277b105267124b5f7e870feab30fb6b8a91

#### OTHER ####

mktun:
	$(CC) $(WARN) mktun.c -o mktun

hostname.h: hostname.sh
	bash hostname.sh


#### DEBUG ####

tuntest.pre: $(OBJECTS) client.c
	$(CC) $(OBJECTS) client.c $(CFLAGS) -E -o tuntest.pre

motecomm.pre: motecomm.c motecomm.h motecomm.sizes.h
	$(CC) $(FLAGS) -c motecomm.c -E -o motecomm.pre

<<<<<<< HEAD
hostname.h: hostname.sh
	bash hostname.sh
=======
motecomm: motecomm.c motecomm.h motecomm.sizes.h
	$(CC) $(FLAGS) -DSTANDALONE motecomm.c $(EXTERNALS) -o motecomm

>>>>>>> 1e7e2277b105267124b5f7e870feab30fb6b8a91

#### PHONIES ####
clean:
	rm *.o hostname.h tuntest > /dev/null 2>&1 ; echo cleaned.

diag:
	@echo TOSROOT: $(TOSROOT) INCLUDE: $(INCLUDE)

autogen: hostname.h

force: clean tuntest

.PHONY: clean diag force autogen $(EXTRAPHONIES)
.SILENT: clean $(NOPS)
