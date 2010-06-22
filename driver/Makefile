
INCLUDE = -I$(TOSROOT)/support/sdk/c/blip -I$(TOSROOT)/support/sdk/c/lib6lowpan -I$(TOSROOT)/tos/types
CFLAGS = -D_GNU_SOURCE

tuntest: tun_dev.o main.c
	gcc tun_dev.o main.c $(CFLAGS) $(INCLUDE) -o tuntest

tun_dev.o: tun_dev.c
	gcc -c tun_dev.c $(CFLAGS) $(INCLUDE) -o tun_dev.o

clean:
	rm *.o tuntest

.PHONY: clean
