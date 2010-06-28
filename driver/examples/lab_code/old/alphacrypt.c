// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]

/**
 * @file	alphacrypt.c
 * @brief	Alpha Crypto Tool
 *		This tool generates RSA key pairs, which can be used
 *		to secure the handshake in the Alpha protocol
 * @author	Florian Weingarten <flo@hackvalue.de>
 */

#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rand.h>

#include "alphacrypt.h"

// How many bytes maximum to seed the random number generator?
#define RSA_SEED_SIZE	1024

// Timestamp format (gets written in the output files as commentary)
#define TIME_STR	"%H:%M:%S %d-%m-%Y"

void print_usage(char *me) {
//[[
	printf("Alpha Crypto Tool -- %s [-g] [-o prefix] [-v]\n", me);
	printf(
		" -g	Generate RSA private/public key pair for Alpha\n"
		" -o	Set the prefix for the output files (default: %s)\n"
		" -v	Be verbose\n"
		" -h	Show this help message\n",
		FILE_PREFIX
	);	
} //]]

int main(int argc, char *argv[]) {

	int generate = 0;
	int verbose = 0;
	char *pubfile = NULL;	
	char *privfile = NULL;

	int c;
	while((c = getopt(argc,argv,"o:ghv")) != -1) {
		switch(c) {
			case 'o':
				pubfile = malloc(strlen(optarg)+1+strlen(FILE_PUB_EXT)+1);
				privfile = malloc(strlen(optarg)+1+strlen(FILE_PRIV_EXT)+1);
				if(pubfile == NULL || privfile == NULL) {
					fprintf(stderr, "malloc failed: %s\n", strerror(errno));
					exit(EXIT_FAILURE);
				}
				sprintf(pubfile,  "%s.%s", optarg, FILE_PUB_EXT);
				sprintf(privfile, "%s.%s", optarg, FILE_PRIV_EXT);				
				break;
			case 'g':
				generate = 1;
				break;
			case 'v':
				verbose++;
				break;
			case 'h':
			default:
				print_usage(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if(pubfile == NULL || privfile == NULL) {
		pubfile = malloc(strlen(FILE_PREFIX)+1+strlen(FILE_PUB_EXT)+1);
		privfile = malloc(strlen(FILE_PREFIX)+1+strlen(FILE_PRIV_EXT)+1);
		if(pubfile == NULL || privfile == NULL) {
			fprintf(stderr, "malloc failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		sprintf(pubfile, "%s.%s", FILE_PREFIX, FILE_PUB_EXT);
		sprintf(privfile, "%s.%s", FILE_PREFIX, FILE_PRIV_EXT);
	}

	if(argc == 1 || !generate) {
		print_usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	char seed[RSA_SEED_SIZE];
	printf("Please enter up to %d random characters for seeding the RSA key generator\n", RSA_SEED_SIZE);
	fgets(seed, RSA_SEED_SIZE, stdin);

	// Seed the OpenSSL random number generator
	RAND_seed(seed, RSA_SEED_SIZE);

	// Generate the RSA public/private key pair
	RSA *pair = RSA_generate_key(RSA_MOD_BITS, RSA_EXP, NULL, NULL);

	if(pair == NULL) {
		fprintf(stderr, "Generating key pair failed: %s\n", ERR_error_string(ERR_get_error(), NULL));
		exit(EXIT_FAILURE);
	}

	if(verbose > 0) {
		puts("\nGenerated key pair:");
		RSA_print_fp(stdout, pair, 3);
		puts("");
		puts("");
	}

	time_t t = time(NULL);
	struct tm *tmp = localtime(&t);
	char timestamp[100];
	strftime(timestamp, 100, TIME_STR, tmp);
	char hostname[512];
	gethostname(hostname, 512);

	printf("Saving public key to %s\n", pubfile);

	FILE *pub = fopen(pubfile, "w");
	if(pub == NULL) {
		fprintf(stderr, "Failed to open file '%s' for writing: %s\n", pubfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf(pub, "# ALPHA RSA PUBLIC KEY\n");
	fprintf(pub, "# Generated on %s at %s\n", hostname, timestamp);
	fprintf(pub, "# Modulus length: %d bits\n", RSA_MOD_BITS);
	fprintf(pub, "e = %s\n", BN_bn2hex(pair->e));
	fprintf(pub, "N = %s\n", BN_bn2hex(pair->n));
	fprintf(pub, "# END OF ALPHA RSA PUBLIC KEY\n");
	fclose(pub);

	printf("Saving private key to %s\n", privfile);

	FILE *priv = fopen(privfile, "w");
	if(priv == NULL) {
		fprintf(stderr, "Failed to open file '%s' for writing: %s\n", privfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf(priv, "# ALPHA RSA PRIVATE KEY\n");
	fprintf(priv, "# Generated on %s at %s\n", hostname, timestamp);
	fprintf(priv, "# Modulus length: %d bits\n", RSA_MOD_BITS);
	fprintf(priv, "d = %s\n", BN_bn2hex(pair->d));
	fprintf(priv, "N = %s\n", BN_bn2hex(pair->n));
	fprintf(priv, "p = %s\n", BN_bn2hex(pair->p));
	fprintf(priv, "q = %s\n", BN_bn2hex(pair->q));
	fprintf(priv, "dmp1 = %s\n", BN_bn2hex(pair->dmp1));
	fprintf(priv, "dmq1 = %s\n", BN_bn2hex(pair->dmq1));
	fprintf(priv, "iqmp = %s\n", BN_bn2hex(pair->iqmp));
	fprintf(priv, "# END OF ALPHA RSA PRIVATE KEY\n");
	fclose(priv);

/*	unsigned char from[] = "Hallo Werner";
	unsigned char to[RSA_size(pair)];
	RSA_public_encrypt(13, from, to, pair, RSA_PKCS1_PADDING);

	printf("%d\n", RSA_size(pair));

	unsigned char out[RSA_size(pair)];
	RSA_private_decrypt(RSA_size(pair), to, out, pair, RSA_PKCS1_PADDING);

	printf("%s\n", out);
*/
/*	int i;
	for(i=0; i<RSA_size(pair); i++) {
		printf("%x", to[i]);
	}
*/

	free(pubfile);
	free(privfile);

	RSA_free(pair);
	RAND_cleanup();

	return EXIT_SUCCESS;

}
