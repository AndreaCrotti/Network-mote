// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]

#include <stdlib.h>
#include <check.h>

#include "../defines.h"
#include "../xmalloc.h"
#include "../list.h"

/***** List Tests *****/
//[[
static list_t *list = NULL;

void setup_unchecked_list(void) {
	list = list_new(NULL);
}

void teardown_unchecked_list(void) {
	//list_free(list);
	list = NULL;
}

/*!
	Inserts some elements at the front of the list 
	and checks wheter they were added to the list 
	using list_peek_front().
 */
START_TEST (test_list_insert_front) {
	size_t size_of_list = list_size(list);
	unsigned char data_front[5] = {4, 7, 2, 5, 8};
	unsigned char data_middle[4] = {5, 6, 3, 0};
	unsigned char data_end[5] = {32, 5, 7, 21, 45};
	list_push_front(list, data_front, 5);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	size_t len;
	unsigned char* value = list_peek_front(list, &len);
	fail_unless(len == 5);
	int i;
	for(i = 0; i < 5; ++i){
		fail_unless(data_front[i] == value[i]);
	}
	list_push_front(list, data_middle, 4);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	value = list_peek_front(list, &len);
	fail_unless(len == 4);
	for(i = 0; i < 4; ++i){
		fail_unless(data_middle[i] == value[i]);
	}
	list_push_front(list, data_end, 5);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	value = list_peek_front(list, &len);
	fail_unless(len == 5);
	for(i = 0; i < 5; ++i){
		fail_unless(data_end[i] == value[i]);
	}
} END_TEST

/*!
	Inserts some elements at the back of the list 
	and checks wheter they were added to the list 
	using list_peek_back().
 */
START_TEST (test_list_insert_back) {
	size_t size_of_list = list_size(list);
	unsigned char data_end[3] = {5, 7, 3};
	unsigned char data_very_end[7] = {3, 5, 1, 8, 5, 44, 21};
	list_push_back(list, data_end, 3);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	size_t len;
	unsigned char* value = list_peek_back(list, &len);
	fail_unless(len == 3);
	int i;
	for(i = 0; i < 3; ++i){
		fail_unless(data_end[i] == value[i]);
	}
	list_push_back(list, data_very_end, 7);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	value = list_peek_back(list, &len);
	fail_unless(len == 7);
	for(i = 0; i < 7; ++i){
		fail_unless(data_very_end[i] == value[i]);
	}
} END_TEST

/*!
	Adds some elements to the list and pops them 
	from the list using list_pop_front().
 */
START_TEST (test_list_pop_front) {
	unsigned char data_front[4] = {5, 3, 9, 7};
	unsigned char data_very_front[3] = {21, 45, 43};
	size_t size_of_list = list_size(list);
	list_push_front(list, data_front, 4);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	list_push_front(list, data_very_front, 3);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	size_t len;
	unsigned char* value = list_pop_front(list, &len);
	size_of_list--;
	fail_unless(size_of_list == list_size(list));
	fail_unless(len == 3);
	int i;
	for(i = 0; i < 3; ++i){
		fail_unless(data_very_front[i] == value[i]);
	}
	value = list_pop_front(list, &len);
	size_of_list--;
	fail_unless(size_of_list == list_size(list));
	fail_unless(len == 4);
	for(i = 0; i < 4; ++i){
		fail_unless(data_front[i] == value[i]);
	}
} END_TEST 

/*!
	Adds some elements to the list and pops them 
	from the list using list_pop_back().
 */
START_TEST (test_list_pop_back) {
	unsigned char data_back[3] = {41, 65, 47};
	unsigned char data_very_back[4] = {55, 87, 48, 96};
	size_t size_of_list = list_size(list);
	list_push_back(list, data_back, 3);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	list_push_back(list, data_very_back, 4);
	size_of_list++;
	fail_unless(size_of_list == list_size(list));
	size_t len;
	unsigned char* value = list_pop_back(list, &len);
	size_of_list--;
	fail_unless(size_of_list == list_size(list));
	fail_unless(len == 4);
	int i;
	for(i = 0; i < 4; ++i){
		fail_unless(data_very_back[i] == value[i]);
	}
	value = list_pop_back(list, &len);
	size_of_list--;
	fail_unless(size_of_list == list_size(list));
	fail_unless(len == 3);
	for(i = 0; i < 3; ++i){
		fail_unless(data_back[i] == value[i]);
	}
} END_TEST

/*!
	Performs several test on the list_iterator_t.
	These include moving from the begin to the end 
	and reverse and also calling list_erase().
*/
START_TEST (test_list_iterator) {
	list_iterator_t* it;
	//delete the entire list
	for(it = list_begin(list); !list_iterator_at_end(it); list_erase(it));
	fail_unless(list_size(list) == 0);
	unsigned char some_data[2] = {5, 7};
	unsigned char some_more_data[3] = {7, 3, 5};
	list_push_back(list, some_data, 2);
	list_push_back(list, some_more_data, 3);

	it = list_begin(list);
	fail_unless(list_iterator_at_begin(it));
	size_t size;
	unsigned char* value = list_iterator_get(it, &size);
	int i;
	for(i = 0; i < 2; ++i){
		fail_unless(value[i] == some_data[i]);
	}
	list_iterator_next(it);
	value = list_iterator_get(it, &size);
	for(i = 0; i < 3; ++i){
		fail_unless(value[i] == some_more_data[i]);
	}
	list_iterator_next(it);
	fail_unless(list_iterator_at_end(it));
	list_iterator_previous(it);
	fail_unless(!list_iterator_at_end(it));
	list_iterator_previous(it);
	fail_unless(list_iterator_at_begin(it));
	value = list_iterator_get(it, &size);
	for(i = 0; i < 2; ++i){
		fail_unless(value[i] == some_data[i]);
	}
	list_iterator_free(it);
} END_TEST
//]]

/***** List Test Suite *****/
Suite * create_list_suite (void) {
	Suite *s = suite_create("list");

	TCase *tc_list = tcase_create("List");
	tcase_add_test(tc_list, test_list_insert_front);
	tcase_add_test(tc_list, test_list_insert_back);
	tcase_add_test(tc_list, test_list_pop_front);
	tcase_add_test(tc_list, test_list_pop_back);
	tcase_add_test(tc_list, test_list_iterator);
	suite_add_tcase(s, tc_list);
	tcase_add_unchecked_fixture(tc_list, setup_unchecked_list, teardown_unchecked_list);

	return s;
}
