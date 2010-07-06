BLIP = $(TOSROOT)/support/sdk/c/blip
SF = $(TOSROOT)/support/sdk/c/sf

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types -I$(SF)
CFLAGS = -D_GNU_SOURCE -DPC
FLAGS = -Wall -Wextra

OBJECTS = tunnel.o split.o #config.o tun_dev.o routing.o
#OBJECTS = 
#For external libraries
EXTERNALS = $(SF)/sfsource.o
SRC = client.c

all: clean tuntest

# TODO: refactoring of those redundant objects
tuntest: $(OBJECTS) client.c
	gcc $(FLAGS) $(OBJECTS) $(EXTERNALS)  client.c $(CFLAGS) $(INCLUDE) -o tuntest

tuntest.pre: $(OBJECTS) client.c
	gcc $(FLAGS) $(OBJECTS) client.c $(CFLAGS) $(INCLUDE) -E -o tuntest.pre

tunnel.o: tunnel.c
	gcc $(FLAGS) -c tunnel.c $(CFLAGS) $(INCLUDE) -o tunnel.o

tun_dev.o: tun_dev.c
	gcc $(FLAGS) -c tun_dev.c $(CFLAGS) $(INCLUDE) -o tun_dev.o

routing.o: routing.c
	gcc $(FLAGS) -c routing.c $(CFLAGS) $(INCLUDE) -o routing.o

split.o: split.c
	gcc -lm -c split.c -o split.o

#config.o: config.c
#	gcc $(FLAGS) -c config.c $(CFLAGS) $(INCLUDE) -o config.o

clean:
	rm -f *.o tuntest

diag:
	@echo TOSROOT: $(TOSROOT) INCLUDE: $(INCLUDE)

.PHONY: clean diag
