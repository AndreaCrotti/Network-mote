SHELL=/bin/bash
CC=gcc
QUIET_CC = @echo '    ' CC $@;

VPATH=lib
HEADERS=$(wildcard *.h)
SRC=../digest.c ../xmalloc.c ../tools.c ../list.c test_list.c test_digest.c

CFLAGS=-lm -lcrypto -lcheck

debug: CFLAGS += -ggdb3 -funit-at-a-time -Wall 
debug: CFLAGS += -Wextra -Wredundant-decls -Wstrict-prototypes -Wdisabled-optimization 
debug: CFLAGS += -Wpointer-arith -Wundef -Wno-deprecated-declarations -Wwrite-strings -Wcast-qual -Wno-unused-parameter
debug: CFLAGS += -Winline
debug: tests

tests: tests.c $(SRC) $(HEADERS)
	$(QUIET_CC)$(CC) $(CFLAGS) $(SRC) $< -o $@

clean:
	@echo '   CLEAN .'
	@rm -rf test_suite

todo:
	@( grep --color -r -n -E "(TODO|FIXME)" *.c *.h filter )

.PHONY: clean todo all
