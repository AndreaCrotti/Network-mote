SF = $(TOSROOT)/support/sdk/c/sf
SHARED = ../shared

# using c99 C standard, for example for "for (int i = 0)..."
CC = gcc
STD = -std=c99

# 1:ERROR 2:WARNING 4:NOTE 8:INFO 16:DEBUG 128:VAR_DUMP
LOG_LEVEL := '(1|2|4|8|16|128)'

PACKET_TYPE = -DCOMPRESSION_ENABLED=1
INCLUDE = -I$(TOSROOT)/tos/types -I$(SF) -I$(SHARED) -I.
LOW6PAN_CARRIED=102
CFLAGS = -D_GNU_SOURCE -DPC -DTOSH_DATA_LENGTH=$(LOW6PAN_CARRIED) -DCLIENT -DDEBUG $(PACKET_TYPE) -DLOG_LEVEL=$(LOG_LEVEL)
WARN = -Wall -Wextra
DEBUG = -ggdb -O0 -pg -fno-omit-frame-pointer
FLAGS = $(WARN) $(INCLUDE) $(CFLAGS) $(DEBUG) $(STD)

HEADERS = util.h motecomm.sizes.h config.h hostname.h
TARGETS := gateway gateway-sf client
MAINS := $(addsuffix %.c,$(TARGETS))
CODEFILES := $(filter-out %.test.c,$(wildcard *.c))
OBJECTS := $(patsubst %.c,%.o,$(CODEFILES))
OBJECTS_NOMAIN := $(filter-out $(addsuffix %.o,$(TARGETS)),$(OBJECTS))
TEST_TARGETS := $(patsubst tests/%.test.c,tests/%,$(wildcard tests/*.test.c))
NICETT := $(notdir $(TEST_TARGETS))
IMPLIED_HEADERS := $(patsubst %.c,%.h,$(CODEFILES))
NOPS :=

#For external libraries
EXTERNALS := $(SF)/sfsource.o $(SF)/serialsource.o /usr/lib/libz.so
EXTRAPHONIES = 

#TIME := time -f '\t%E' --
TIME := 

#### RULES ####

all: autogen $(TARGETS) $(TEST_TARGETS) $(NOPS)

$(TARGETS): %: %.o $(OBJECTS_NOMAIN) $(HEADERS) Makefile
	@echo "Linking executable file <$@>."
	@$(TIME) $(CC) $(FLAGS) $(OBJECTS_NOMAIN) $(EXTERNALS) $< -o $@

$(OBJECTS): %.o: %.c %.h $(HEADERS) Makefile
	@echo    "Compiling object file <$@>."
	@$(TIME) $(CC) $(FLAGS) -c $< -o $@

$(TEST_TARGETS): tests/%: tests/%.test.c $(OBJECTS_NOMAIN) Makefile
	@echo "Linking test executable file <$@>."
	@$(TIME) $(CC) $(FLAGS) $(OBJECTS_NOMAIN) $(EXTERNALS) $< -o $@

$(NICETT): %: tests/%

$(IMPLIED_HEADERS): %.h:
	@[ -e $@ ] || touch $@

$(NOPS): %.o: %.c %.h
	@echo "WARNING: NOT MAKING TARGET '$@'!" && echo "         file(s) '$?' was(were) modified but '$@' is in NOPS:" && echo "         $(NOPS)" && touch $@

doc:
	pdflatex ../README.tex --output-directory ../
	doxygen

#### OTHER ####

hostname.h: hostname.sh
	@echo "Creating <hostname.h>."
	@bash hostname.sh


#### DEBUG ####

# whatever changes we have to recompile the tests
$(patsubst %.o,%.pre,$(OBJECTS)): %.pre: %.c %.h $(HEADERS) Makefile
	@echo    "Creating preprocessed file <$@>."
	@$(TIME) $(CC) $(FLAGS) -E $< -o $@

#### PHONIES ####
clean:
	@echo "Cleaning."
	@rm -f *.o hostname.h $(TARGETS) $(TEST_TARGETS) > /dev/null 2>&1; true

tests: $(TEST_TARGETS)
	@$(patsubst %, LAST_TEST='%' && echo "Running test file <$$LAST_TEST>." && ./$$LAST_TEST &&, $(TEST_TARGETS)) true


diag:
	@echo -e " TOSROOT: $(TOSROOT)\n INCLUDE: $(INCLUDE)\n OBJECTS: $(OBJECTS)\n OBJECTS_NOMAIN: $(OBJECTS_NOMAIN)\n CODEFILES: $(CODEFILES)"

run: client
	@motelist -c | sed 's/^[^,]*,\([^,]*\),.*/echo Using mote at \1 \&\& .\/client \1/' | head -n 1 | bash

run-gw: gateway
	@motelist -c | sed 's/^[^,]*,\([^,]*\),.*/echo Using mote at \1 \&\& .\/gateway \1 eth0/' | head -n 1 | bash

ping:
	@if [ $$(id -u) = 0 ]; then ping -fbi 0.1 -s 1400 10.0.0.2; else echo "You are not root, so I will ping slowly with 1."; ping -fbi 1 10.0.0.2; fi

autogen: hostname.h

force: clean all

.PHONY: clean diag force autogen run ping doc test tests $(EXTRAPHONIES) $(NICETT)
