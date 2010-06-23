BLIP = $(TOSROOT)/support/sdk/c/blip

INCLUDE = -I$(BLIP) -I$(BLIP)/lib6lowpan -I$(TOSROOT)/tos/types
CFLAGS = -D_GNU_SOURCE -DPC
FLAGS = -Wall

OBJECTS = tun_dev.o
#OBJECTS = 

tuntest: $(OBJECTS) main.c
	gcc $(FLAGS) $(OBJECTS) main.c $(CFLAGS) $(INCLUDE) -o tuntest

tuntest.pre: $(OBJECTS) main.c
	gcc $(FLAGS) $(OBJECTS) main.c $(CFLAGS) $(INCLUDE) -E -o tuntest.pre

tun_dev.o: tun_dev.c
	gcc $(FLAGS) -c tun_dev.c $(CFLAGS) $(INCLUDE) -o tun_dev.o

clean:
	rm -f *.o tuntest

diag:
	@echo TOSROOT: $(TOSROOT) INCLUDE: $(INCLUDE)

.PHONY: clean diag
