############################################################
# Makefile for ALPHA                                       #
# - Johannes Gilger <Johannes.Gilger@RWTH-Aachen.de>       #
# - Florian Weingarten <Florian.Weingarten@RWTH-Aachen.de> #
############################################################

SHELL=/bin/bash
CC=gcc
INSTALL=install

BUILD=`date +'%Y%m%d-%H%M%S'`
SVNREV=$(shell LC_ALL=POSIX svn info 2> /dev/null|grep Revision|awk {'print $$2'})
GITREV=$(shell git log --pretty=tformat:"%h" -1 2>/dev/null|sed -e "s/ //g")
LINES=`LC_ALL=POSIX wc -l *.{c,h}| grep total|awk {'print $$1'}`
ARCH:=$(shell uname)
CPU:=$(shell uname -m)
RELEASE:=$(shell uname -r)
QUIET_CC = @echo '    ' CC $@;

VPATH=lib
HEADERS=$(wildcard *.h) $(wildcard lib/*.h)
SUBDIRS=filter tests
SRC=system.c protocol.c signal.c host.c tools.c digest.c key.c hashchain.c  \
hashtree.c application.c alpha.c list.c extended_math.c alpha_n.c alpha_c.c  \
alpha_m.c control.c lib/iniparser.c lib/dictionary.c association.c  \
timemanager.c ring_buffer.c control_association.c cache_tree.c xmalloc.c  \
alpha_z.c

prefix = $(HOME)
sbindir = $(prefix)/sbin
mandir = $(prefix)/share/man

ifeq ($(ARCH),Darwin)
	CFLAGS += -gstabs -DMACOSX -fnested-functions
else
	CFLAGS += -Wmissing-prototypes
endif

ifeq ($(RELEASE),9.7.0) #TODO: This does not work for the OS-X gcc...
	#CLFAGS += -march=core2
endif

ifeq ($(CPU),x86_64)
	CFLAGS += -march=nocona
endif

ifeq ($(CPU),arm)
	CFLAGS += -march=armv6
else
	CFLAGS += -Wshadow
endif

ifeq ($(SVNREV),)
ifeq ($(GITREV),)
	REVISION = "unversioned"
else
	REVISION = "git-$(GITREV)"
endif
else	
	REVISION = "svn-$(SVNREV)"
endif
CFLAGS += -DREVISION=\"${REVISION}\"

all: debug

debug: CFLAGS += -DDEBUG -ggdb3 -funit-at-a-time -Wall 
debug: CFLAGS += -Wextra -Wredundant-decls -Wstrict-prototypes -Wdisabled-optimization 
debug: CFLAGS += -Wpointer-arith -Wundef -Wno-deprecated-declarations -Wwrite-strings -Wcast-qual -Wno-unused-parameter
# -Winline doesn't give any errors unless you compile with -O
debug: CFLAGS += -Winline
debug: alpha alphacontrol

# DNDEBUG for getting rid of assert(), -U_FORTIFY_SOURCE for warn_unused_result gcc bug
final: CFLAGS += -O2 -DNDEBUG -U_FORTIFY_SOURCE
final: CFLAGS += -finline-functions
#final: -Winline		# winy inlining
final: CFLAGS += -fwhole-program -combine -ffast-math
final: alpha alphacontrol

alpha: CFLAGS += -lm -lcrypto
alpha: $(SRC) $(HEADERS) system_tun.c system_divert.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(SRC) -o $@

alphacontrol: alphacontrol.c
	$(QUIET_CC)$(CC) $(CFLAGS) $< -o $@

$(SUBDIRS):
	@$(MAKE) -s -C $@

clean:
	@echo '   CLEAN .'
	@rm -f alpha test *.o ../tests/*.o gmon.out profiling-output.txt alpha.sock alpha.log alphacontrol tests/hashtest
	@echo '   CLEAN doc/'
	@$(MAKE) -s -C doc clean
	@echo '   CLEAN filter/'
	@$(MAKE) -s -C filter clean

doc: $(SRC) $(HEADERS)
	@$(MAKE) -s -C doc
	@echo '    DOC doc/'
	@doxygen .dox_conf &> /dev/null

keys:
	@echo "Generating private/public keys as private_key.pem and public_key.pem"
	@(openssl dsaparam -inform PEM -outform PEM -out dsaparam.arg -text -genkey 1024 &> /dev/null)
	@(openssl gendsa -out private_key.pem dsaparam.arg &> /dev/null)
	@(openssl dsa -pubout -in private_key.pem -out public_key.pem &> /dev/null)
	@(openssl dsa -text -in private_key.pem 2> /dev/null)
	@rm -rf dsaparam.arg

install: alpha alphacontrol
	$(INSTALL) -d -m 755 $(prefix)
	$(INSTALL) -d -m 755 $(sbindir)
	$(INSTALL) -d -m 755 $(mandir)
	$(INSTALL) -d -m 755 $(mandir)/man1
	$(INSTALL) alpha $(sbindir)
	$(INSTALL) alphacontrol $(sbindir)
	$(INSTALL) doc/alpha.1 $(mandir)/man1

#######################
# Development targets #
#######################
profiling: CFLAGS += -pg
profiling: final

profiling-output:
	gprof -b alpha gmon.out > profiling-output.txt

strip:
	strip alpha

tags: $(HEADERS) $(SRC)
	@echo '     Making tags'
	@ctags *.c *.h filter/*.c filter/*.h

todo:
	@( grep --color -r -n -E "(TODO|FIXME)" *.c *.h filter )

lines:
	@echo -e "Alpha lines:\n------------"
	@( wc -l *.c *.h Makefile | sort -gr )
	@echo -e "Filter lines:\n-------------"
	@( wc -l filter/*.{c,h,sh}| sort -gr )

top-wtf: tags
	@echo -e "\nTop 40 function names by length\n-------------------------------"
	@awk '{print length($$1), $$1}' < tags | sort -rn | awk {'print $$2'} | head -n 40 | nl|sed -e "s/    //" -e "s/\t/  /"

.PHONY: strip tags clean doc lines todo filter install all keypair top-wtf
