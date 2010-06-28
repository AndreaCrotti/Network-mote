CC=gcc
QUIET_CC = @echo '    ' CC $@;

all: evil debug

# Final flags
CFLAGS += -Wall -lcrypto -I/usr/include/libipq

final: CFLAGS += -O2 -DNDEBUG -U_FORTIFY_SOURCE
final: CFLAGS += -finline-functions -Winline
final: CFLAGS += -fwhole-program -combine -ffast-math
final: alphafilter

# Tobis development flags
debug: CFLAGS += -DDEBUG -ggdb3 -funit-at-a-time 
debug: CFLAGS += -Wextra -Winline -Wredundant-decls -Wstrict-prototypes -Wdisabled-optimization -Wpointer-arith -Wundef -Wno-deprecated-declarations -Wwrite-strings -Wcast-qual -Wshadow -Wno-unused-parameter
debug: alphafilter

alphafilter: alphafilter.c client.c ../tools.c ../xmalloc.c ../digest.c
	$(QUIET_CC)$(CC) $(CFLAGS) ../key.c ../tools.c ../xmalloc.c ../digest.c ../ring_buffer.c ../extended_math.c ../cache_tree.c ../hashtree.c ../control_association.c ../host.c ../association.c ../hashchain.c ../alpha_m.c ../alpha_n.c ../alpha_c.c ../alpha_z.c ../list.c ../protocol.c ../timemanager.c ../lib/dictionary.c ../lib/iniparser.c ../application.c ../system.c association_relay.c association_relay_alpha_n.c association_relay_alpha_m.c association_relay_alpha_c.c association_relay_alpha_z.c alphafilter.c client.c -o alphafilter -lipq -lm

evil: evil.c
	$(QUIET_CC)$(CC) $(CFLAGS) $< -o $@ -lipq

clean:
	rm -f filter evil alphafilter

.PHONY: clean
