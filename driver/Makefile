BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF)
CFLAGS = -D_GNU_SOURCE -DPC
FLAGS = -Wall -Wextra

OBJECTS = tunnel.o split.o motecomm.o #config.o tun_dev.o routing.o
#OBJECTS = 
#For external libraries
EXTERNALS = $(SF)/sfsource.o $(SF)/serialsource.o
SRC = client.c

# all: clean tuntest

# TODO: refactoring of those redundant objects
tuntest: autogen $(OBJECTS) client.c
	gcc $(FLAGS) $(OBJECTS) $(EXTERNALS)  client.c $(CFLAGS) $(INCLUDE) -o tuntest

tuntest.pre: $(OBJECTS) client.c
	gcc $(FLAGS) $(OBJECTS) client.c $(CFLAGS) $(INCLUDE) -E -o tuntest.pre

tunnel.o: tunnel.c
	gcc $(FLAGS) -c tunnel.c $(CFLAGS) $(INCLUDE) -o tunnel.o

tun_dev.o: tun_dev.c
	gcc $(FLAGS) -c tun_dev.c $(CFLAGS) $(INCLUDE) -o tun_dev.o

routing.o: routing.c
	gcc $(FLAGS) -c routing.c $(CFLAGS) $(INCLUDE) -o routing.o

motecomm.o: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -c motecomm.c $(CFLAGS) $(INCLUDE) -o motecomm.o

motecomm: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -DSTANDALONE motecomm.c $(EXTERNALS) $(CFLAGS) $(INCLUDE) -ggdb -O0 -o motecomm

motecomm.pre: motecomm.c motecomm.h motecomm.sizes.h
	gcc $(FLAGS) -c motecomm.c $(CFLAGS) $(INCLUDE) -E -o motecomm.pre

split.o: split.c
	gcc -lm -c split.c -o split.o

#config.o: config.c
#	gcc $(FLAGS) -c config.c $(CFLAGS) $(INCLUDE) -o config.o

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
