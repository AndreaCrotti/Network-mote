BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf
6LOWPANCARRIED = 128

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF)
# this changes the size of the message (found http://www.tinyos.net/tinyos-2.x/doc/html/tep111.html)
CFLAGS = -D_GNU_SOURCE -DPC -DTOSH_DATA_LENGTH=$(6LOWPANCARRIED)
FLAGS = -Wall -Wextra $(INCLUDE) $(CFLAGS)

OBJECTS = tunnel.o split.o motecomm.o util.o glue.o #config.o tun_dev.o routing.o 
#OBJECTS = 
#For external libraries
EXTERNALS = $(SF)/sfsource.o $(SF)/serialsource.o
SRC = client.c

# all: clean tuntest

# TODO: refactoring of those redundant objects
tuntest: autogen $(OBJECTS) client.c
	gcc $(FLAGS) $(OBJECTS) $(EXTERNALS)  client.c $(CFLAGS) -o tuntest

tuntest.pre: $(OBJECTS) client.c
	gcc $(OBJECTS) client.c $(CFLAGS) -E -o tuntest.pre

tunnel.o: tunnel.c
	gcc $(FLAGS) -c tunnel.c -o tunnel.o

tun_dev.o: tun_dev.c
	gcc $(FLAGS) -c tun_dev.c -o tun_dev.o

routing.o: routing.c
	gcc $(FLAGS) -c routing.c -o routing.o

glue.o: glue.h glue.c
	gcc $(FLAGS) -c glue.c -o glue.o

util.o: util.h util.c
	gcc $(FLAGS) -c util.c -o util.o

motecomm.o: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -c motecomm.c -o motecomm.o

motecomm: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -DSTANDALONE motecomm.c $(EXTERNALS) -ggdb -O0 -o motecomm

motecomm.pre: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -c motecomm.c -E -o motecomm.pre

split.o: split.c
	gcc -lm -c split.c -o split.o

hostname.h: hostname.sh
	bash hostname.sh

clean:
	rm -f *.o hostname.h tuntest

# Makefile diagnostic
diag:
	@echo TOSROOT: $(TOSROOT) INCLUDE: $(INCLUDE)

autogen: hostname.h

force: clean tuntest

.PHONY: clean diag force autogen
