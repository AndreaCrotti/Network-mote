
INCLUDE = -I$(TOSROOT)/support/sdk/c/blip -I$(TOSROOT)/support/sdk/c/lib6lowpan -I$(TOSROOT)/tos/types
CFLAGS = -D_GNU_SOURCE -DPC
FLAGS = -Wall

#OBJECTS = tun_dev.o
OBJECTS = 

tuntest: $(OBJECTS) main.c
	gcc $(FLAGS) $(OBJECTS) main.c $(CFLAGS) $(INCLUDE) -o tuntest

tun_dev.o: tun_dev.c
	gcc $(FLAGS) -c tun_dev.c $(CFLAGS) $(INCLUDE) -o tun_dev.o

clean:
	rm -f *.o tuntest

.PHONY: clean
