SHELL=/bin/bash
CC=gcc
QUIET_CC = @echo '    ' CC $@;

VPATH=lib
HEADERS=$(wildcard *.h) $(wildcard lib/*.h)
SRC=main.c test_list.c ../../list.c ../../xmalloc.c ../../tools.c

test_list: $(SRC) $(HEADERS)
	$(QUIET_CC)$(CC) $(CFLAGS) $(SRC) -o $@

clean:
	@echo '   CLEAN .'
	@rm -rf test_list

.PHONY: clean todo
