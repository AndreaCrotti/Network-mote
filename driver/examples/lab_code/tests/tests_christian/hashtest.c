//#include "digest.h"

#define DEBUG_HASHCHAIN
#include <stdlib.h>
#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "../hashchain.h"
#include "../tools.c"
#include "../digest.c"

int main (const int argc,  char * const argv[]) {


	HC_DEBUG("This program displays the output of the hash chain functions.\n");
	hash_chain_t * my_chain;
	my_chain = hchain_create(&create_digest, 20, 5, 0);
	printf("Printing hash chain\n");
	hchain_print(my_chain);
	HC_DEBUG("Hash length: %d\n", my_chain->hash_length);
	HC_HEXDUMP("Current: ", (char *) hchain_current(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Next: ", (char *) hchain_next(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_DEBUG("\n");

	HC_HEXDUMP("Pop1: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Pop2: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Pop3: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Pop4: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Current: ", (char *) hchain_current(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Next5: ", (char *) hchain_next(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");

	HC_HEXDUMP("Pop5: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Pop6: ", (char *) hchain_pop(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Next6: ", (char *) hchain_next(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");
	HC_HEXDUMP("Current: ", (char *) hchain_current(my_chain), my_chain->hash_length);
	HC_DEBUG("\n");


	hchain_free(my_chain);
	return 1;
}

