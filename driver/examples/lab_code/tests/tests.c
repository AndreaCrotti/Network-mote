// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <check.h>

#include "tests.h"

int main (void) {
	int number_failed;
	SRunner *sr;

	sr = srunner_create(NULL);

	srunner_add_suite(sr, (Suite *) create_list_suite());
	srunner_add_suite(sr, (Suite *) create_digest_suite());

	//srunner_set_fork_status(sr, CK_NOFORK);
	srunner_run_all(sr, CK_VERBOSE);
	number_failed=srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
