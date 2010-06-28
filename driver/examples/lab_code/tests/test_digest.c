// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "../digest.h"
#include "../defines.h"
#include "../xmalloc.h"

/***** Digest Tests *****/
//[[
unsigned char *message, *digest;

void setup_digest(void) {
	message = xmalloc(21);
	memcpy(message, "01234567890123456789", 21);
	digest = xmalloc(HASHSIZE);
	memset(digest, 0, HASHSIZE);
	create_digest(message,0,digest);
}

void teardown_digest(void) {
	xfree(message);
	message = NULL;
	xfree(digest);
	digest = NULL;
}

START_TEST (test_create_digest) {
	// obviously this is for SHA1
	unsigned char expected[21] = "\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09";
	
	fail_unless(memcmp(digest, expected, 20) == 0, "create_digest not correct!");
} END_TEST

START_TEST (test_display_digest) {
	char *message_digest = xmalloc(HASHSIZE*2+1);
	digeststr(digest, message_digest);

	// obviously this is for SHA1
	char expected[41] = "da39a3ee5e6b4b0d3255bfef95601890afd80709";

	fail_unless(strncmp(expected, message_digest, 41) == 0, "digeststr not correct!");
} END_TEST

START_TEST (test_hash_chain) {
	hash_chain(message, 100, digest);
	
	// obviously this is for SHA1
	unsigned char expected[21] = "\x7a\xe9\xdf\x3b\x21\x0a\xec\xc0\x68\x1c\x27\x57\xfc\x89\x86\x3d\xe4\x6f\x51\xfd";

	fail_unless(memcmp(expected, digest, 20) == 0, "hash_chain not correct!");
} END_TEST
//]]

/***** Digest Test Suite *****/
Suite * create_digest_suite (void) {
	Suite *s = suite_create("digest");

	TCase *tc_digest = tcase_create("Digest");
	tcase_add_test(tc_digest, test_create_digest);
	tcase_add_test(tc_digest, test_display_digest);
	tcase_add_test(tc_digest, test_hash_chain);
	suite_add_tcase(s, tc_digest);
	tcase_add_checked_fixture(tc_digest, setup_digest, teardown_digest);

	return s;
}
