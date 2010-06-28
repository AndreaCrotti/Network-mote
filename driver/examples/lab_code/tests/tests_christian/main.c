
#include "test_list.h"
#include <stdio.h>

int main(){
	test_list_init();
	test_list_insert_front();
	test_list_insert_back();
	test_list_pop_front();
	test_list_pop_back();
	test_list_iterator();
	test_list_clean();
	printf("Test Run Done\n");
	return 0;
}
