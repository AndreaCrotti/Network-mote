
/*!
	@file test_list.h Tests diverse functions for list.c
	
	This file tests functions of the list.c using SAFE COPY off 
	and AUTO FREE off as well.
 */



#ifndef __TEST_LIST_H
#define __TEST_LIST_H

int test_list_init();

int test_list_clean();

void test_list_insert_front();

void test_list_insert_back();

void test_list_pop_front();

void test_list_pop_back();

void test_list_iterator();


#endif // __TEST_LIST_H

