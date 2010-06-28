// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file list.h
 * @brief Modul for a list
 * the list contains a sentinel element and offers a forward iterator
 *
 * @author Christian Dernehl <christian.dernehl@rwth-aachen.de>
 * @date April 2009
 *
 * \code
 * 	//example (1) for creating a list
 * 	list_t *mylist = list_new(NULL);
 * 	void buf[255];
 * 	list_push_back(mylist, buf, 255);
 * 	list_free(mylist);
 * 	//example (2) for creating a list
 * 	list_t *mylist = list_new(&data_packet_free_func);
 * 	list_set_auto_free(1);
 * 	int i;
 * 	for(i = 0; i < 10; ++i) {
 * 		packet *p = (packet*)malloc(sizeof(packet));
 * 		list_push_back(mylist, p, sizeof(packet));
 * 	}
 * 	list_iterator_t *it;
 * 	for(it = list_begin(mylist); !list_iterator_at_end(it); list_iterator_next(it)) {
 * 	}
 * 	//don't forget to free the iterator!
 * 	list_iterator_free(it);
 *
 * 	// also all p's are freed automatically using data_packet_free_func
 * 	list_free(mylist);
 * 	\endcode
 */

#ifndef __LIST_H_
#define __LIST_H_

#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

//! Pointer which points to the content of a list item
/*!
 * @see list_item_t
 */
typedef unsigned char* content_pointer_t;

//! A function pointer to a function which frees allocated memory
/*!
 * @see list_new(list_free_t)
 */
typedef void (*list_free_t)(void*);

//! A bitset for storing options
/*!
 * @see list_t
 */
#define LIST_SAFE_COPY 0x1
#define LIST_AUTO_FREE 0x2

typedef uint8_t list_options_t;

typedef uint8_t list_iterator_options_t;

//! A list item containing data
typedef struct list_item{
	content_pointer_t data;
	size_t data_len;
	struct list_item *next;
	struct list_item *previous;
} list_item_t;

//! the acutal list
typedef struct list{
	list_item_t *head;
	list_item_t *tail;
	list_item_t *sentinel;
	size_t size;
	//! Bitset containing options for the list
	/*!
		The rightmost bit determines if SAFE COPY
		is turned on. Next to this bit, the
		boolean value AUTO FREE is set.
		The other six bits are not used yet.
	*/
	list_options_t options;
	//! Function pointer to a function which frees the data per element
	list_free_t free_func;
} list_t;

//! A forward list iterator
typedef struct list_iterator{
	//! the current element in the list
	list_item_t *current;
	//! The list in which the iterator "lives"
	list_t *list;
	//! If the iterator is at the begin
	bool at_begin;
	//! If the iterator is at the end
	bool at_end;
} list_iterator_t;

content_pointer_t list_internal_copy_data(const list_t*, content_pointer_t, content_pointer_t, const size_t);

void list_internal_free(list_t*, content_pointer_t);

void list_iterator_internal_check_begin(list_iterator_t*);

void list_iterator_internal_check_end(list_iterator_t*);

void list_iterator_internal_set_begin(list_iterator_t*, const bool value);

void list_iterator_internal_set_end(list_iterator_t*, const bool value);

list_t* list_new(list_free_t);

void list_free(list_t*);

size_t list_size(const list_t*);

list_t* list_push_front(list_t*, content_pointer_t, const size_t);

content_pointer_t list_peek_front(const list_t*, size_t*);

list_t* list_push_back(list_t*, content_pointer_t, const size_t);

content_pointer_t list_peek_back(const list_t*, size_t*);

content_pointer_t list_pop_front(list_t*, size_t*);

content_pointer_t list_pop_back(list_t*, size_t*);

list_iterator_t* list_begin(list_t*);

list_iterator_t* list_iterator_move_begin(list_iterator_t* it);

list_iterator_t* list_end(list_t*);

list_iterator_t* list_iterator_move_end(list_iterator_t* it);

void list_iterator_free(list_iterator_t*);

list_iterator_t* list_iterator_next(list_iterator_t*);

list_iterator_t* list_iterator_previous(list_iterator_t*);

int list_iterator_cmp(const list_iterator_t*, const list_iterator_t*);

bool list_iterator_at_begin(const list_iterator_t*);

bool list_iterator_at_end(const list_iterator_t*);

content_pointer_t list_iterator_get(const list_iterator_t*, size_t*);

list_t* list_iterator_get_list(const list_iterator_t*);

int list_erase(list_iterator_t*);

#endif // __LIST_H_
