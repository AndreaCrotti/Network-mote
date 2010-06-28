// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/*!
 * @file list.c See list.h for documentation
 * @brief See list.h for documentation
 */

#include <stdlib.h>
#include <assert.h>

#include "list.h"
#include "xmalloc.h"

//! Checks wheter the iterator has reached the begin
/*!
 * Sets the specific bit in the options bitset.
 * @param[in] iterator the iterator to be used
 */
void list_iterator_internal_check_begin(list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	// set option if the iterator has reached the begin
	if(iterator->current == iterator->list->head) {
		list_iterator_internal_set_begin(iterator, true);
	} else {
		list_iterator_internal_set_begin(iterator, false);
	}
} //]]

//! Checks wheter the iterator has reached the end
/*!
 * Sets the specific bit in the options bitset.
 * @param[in] iterator the iterator to be used
 */
void list_iterator_internal_check_end(list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	// set option if the iterator has reached the end
	if(iterator->current == iterator->list->tail) {
		list_iterator_internal_set_end(iterator, true);
	} else {
		list_iterator_internal_set_end(iterator, false);
	}
} //]]

//! Sets the iterator option to "at begin"
/*!
 * This function sets the bit in the list_iterator_t
 * options bitset to the given value
 *
 * @param[in] iterator the iterator to be used
 * @param[in] value if true the option for begin is set to true otherwise to false
 */
void list_iterator_internal_set_begin(list_iterator_t* iterator, const bool value) {
//[[
	assert(iterator != NULL);
	iterator->at_begin = value;
} //]]

//! Sets the iterator option to "at end"
/*!
 * This function sets the bit in the list_iterator_t
 * options bitset to the given value
 *
 * @param[in] iterator the iterator to be used
 * @param[in] value if true the option for end is set to true otherwise to false
 */
void list_iterator_internal_set_end(list_iterator_t* iterator, const bool value) {
//[[
	assert(iterator != NULL);
	// set option if value is true
	iterator->at_end = value;
} //]]

//! Creates a new list
/*!
 * Builds a new list. If the option AUTO FREE is set then the given
 * function will be called whenever a item is removed from the list
 * (e.g. due to list_free() or list_erase()).
 *
 * @param free_func the pointer to the function which frees the data content
 * @return NULL in case of failure, otherwise the ptr to the created list
 *
 * @see list_set_auto_free()
 */
list_t* list_new(list_free_t free_func) {
//[[
	list_t *ret_val = xmalloc(sizeof(list_t));
	ret_val->size = 0;

	// set sentinel element
	ret_val->head = ret_val->tail = xmalloc(sizeof(list_item_t));
	ret_val->sentinel = ret_val->head;

	ret_val->head->data_len = 0;
	ret_val->head->data = NULL;

	// set neighboring elements
	ret_val->head->next = ret_val->head;
	ret_val->head->previous = ret_val->head;

	// set free function (or the c std free function)
	ret_val->free_func = free_func == NULL ? free : free_func;

	return ret_val;

} //]]

//! Frees a list
/*!
 * Deletes all data content and frees the list.
 */
void list_free(list_t* list) {
//[[
	if(list == NULL)
		return;
	list_item_t *cur = list->head;
	list_item_t *next = NULL;
	while (cur != list->tail) {
		next = cur->next;
		xfree(cur->data);
		xfree(cur);
		cur = next;
	}
	xfree(list->tail);
	xfree(list);
} //]]

//! Retrieves the size of a list
/*!
 * @param[in] list which list should be checked?
 * @return the size of the given list
 */
inline size_t list_size(const list_t* list) {
//[[
	return list->size;
} //]]


//! Inserts a item at the beginning of the list
/*!
 * @param[in,out] list the list in which the data should be inserted
 * @param[in] data the acutal data
 * @param[in] data_len the size of the data in bytes
 * @return the list if everything went ok, otherwise NULL if a error occured
 */
list_t* list_push_front(list_t* list, content_pointer_t data, const size_t data_len) {
//[[
	assert(list != NULL);
	assert(data != NULL);
	assert(data_len > 0);
	list_item_t *cur = xmalloc(sizeof(list_item_t));

	// set ptr to neighbors
	cur->next = list->head;
	cur->previous = list->sentinel;
	// set neighbor ptrs to this
	list->head->previous = cur;
	list->sentinel->next = cur;
	list->head = cur;

	cur->data = xmalloc(data_len);
	memcpy(cur->data, data, data_len);
	cur->data_len = data_len;
	list->size++;
	return list;
} //]]

//! Retrieves the first element from the list and erases it from the list
/*!
 * Return the data of the first item of the list and removes it. The
 * same will be done as if you store your result from list_peek_first(),
 * call erase() with a iterator pointing to the first item (list_begin())
 * and freeing the iterator.
 *
 * @param[in,out] list the list which should be peeked
 * @param[out] out_size the size of the out_data pointer (must be allocated before)
 *
 * @return a pointer to out_data, NULL in case of failure
 */
content_pointer_t list_pop_front(list_t* list, size_t* out_size) {
//[[
	content_pointer_t out_data = list_peek_front(list, out_size);
	if(out_data == NULL) {
		return NULL;
	}
	// delete first item from list
	list_iterator_t *p = list_begin(list);
	if(p == NULL || !list_erase(p)) {
		list_iterator_free(p);
		return NULL;
	}
	list_iterator_free(p);
	return out_data;
} //]]

//! Peeks the first data item from the list
/*!
 * Return the data of the first element of the list (without removing it)
 *
 * @param[in] list the list which should be peeked
 * @param[out] out_size the size of the out_data pointer (must be allocated before)
 *
 * @return a pointer to out_data, NULL in case of failure
 */
content_pointer_t list_peek_front(const list_t* list, size_t* out_size) {
//[[
	assert(list != NULL);
	assert(out_size != NULL);
	if(list->head == list->sentinel || !list->head->data) {
		return NULL;
	}
	*out_size = list->head->data_len;
	content_pointer_t out_data = xmalloc(*out_size);
	memcpy(out_data, list->head->data, *out_size);
	return out_data;
} //]]

//! Inserts a item at the end of the list
/*!
 * @param[in,out] list The list in which the data should be inserted
 * @param[in] data the acutal data
 * @param[in] data_len The size of the data in bytes
 *
 * @return the list if everything went ok, otherwise NULL if a error occured
 */
list_t* list_push_back(list_t* list, content_pointer_t data, size_t data_len) {
//[[
	assert(list != NULL);
	assert(data != NULL);
	assert(data_len > 0);
	list_item_t *cur = xmalloc(sizeof(list_item_t));

	// set ptr to neighbors
	cur->next = list->sentinel;
	cur->previous = list->tail->previous;
	// set neighbor ptrs to this
	cur->previous->next = cur;
	list->sentinel->previous = cur;
	// check head
	if(list->head == list->sentinel)
		list->head = cur;

	cur->data = xmalloc(data_len);
	memcpy(cur->data, data, data_len);
	cur->data_len = data_len;
	list->size++;

	return list;
} //]]

//! Retrieves the last element from the list and erases it from the list
/*!
 * Return the data of the last item of the list and removes it. The
 * same will be done as if you store your result from list_peek_back(),
 * call erase() with a iterator pointing to the last item (list_end())
 * and freeing the iterator.
 *
 * @param[in,out] list the list which should be peeked
 * @param[out] out_size the size of the out_data pointer (must be allocated before)
 *
 * @return a pointer to out_data, NULL in case of failure
 */
content_pointer_t list_pop_back(list_t* list, size_t *out_size) {
//[[
	content_pointer_t out_data = list_peek_back(list, out_size);
	if(out_data == NULL) {
		return NULL;
	}

	// delete last item from list
	list_iterator_t *p = list_end(list);
	if(!list_erase(p)) {
		list_iterator_free(p);
		return NULL;
	}
	list_iterator_free(p);
	return out_data;
} //]]

//! Peeks the last data item from the list
/*!
 * Return the data of the last element of the list (without removing it)
 * @param[in] list the list which should be peeked
 * @param[out] out_size the size of the out_data pointer (must be allocated before)
 * @return a pointer to out_data, NULL in case of failure
 */
content_pointer_t list_peek_back(const list_t* list, size_t *out_size) {
//[[
	assert(list != NULL);
	assert(out_size != NULL);
	if(list->tail->previous == list->sentinel) {
		return NULL;
	}
	*out_size = list->tail->previous->data_len;
	content_pointer_t out_data = xmalloc(*out_size);
	memcpy(out_data, list->tail->previous->data, *out_size);
	return out_data;
} //]]

//! Creates a list iterator starting at the beginning of the list
/*!
 * @param[in] list the list on which the iterator will work on
 * @return NULL if a error occured, otherwise the iterator
 */
list_iterator_t* list_begin(list_t* list) {
//[[
	assert(list != NULL);
	list_iterator_t *ret_val = xmalloc(sizeof(list_iterator_t));

	ret_val->list = list;
	ret_val->current = list->head;
	list_iterator_internal_check_end(ret_val);
	list_iterator_internal_set_begin(ret_val, true);
	return ret_val;
} //]]

//! Moves a already allocated iterator to the begin of the list
/*!
 * Uses the list pointer from the iterator to reset the iterator.
 * @param[in,out] it the iterator which should be moved to the begin
 * @return the resulting list iterator (which is it in this case)
 */
list_iterator_t* list_iterator_move_begin(list_iterator_t* it) {
//[[
	assert(it != NULL);
	assert(it->list != NULL);
	it->current = it->list->head;
	list_iterator_internal_check_end(it);
	list_iterator_internal_set_begin(it, true);
	return it;
} //]]

//! Creates a list iterator starting at the end of the list
/*!
 * @param[in] list the list on which the iterator will work on
 * @return NULL if a error occured, otherwise the iterator
 */
list_iterator_t* list_end(list_t* list) {
//[[
	assert(list != NULL);
	list_iterator_t *ret_val = xmalloc(sizeof(list_iterator_t));

	ret_val->list = list;
	ret_val->current = list->tail->previous;
	list_iterator_internal_set_end(ret_val, true);
	list_iterator_internal_check_begin(ret_val);
	return ret_val;
} //]]

//! Moves a already allocated iterator to the end of the list
/*!
 * Uses the list pointer from the iterator to reset the iterator.
 *
 * @param[in,out] it the iterator which should be moved to the end
 * @return the resulting list iterator (which is it in this case)
 */
list_iterator_t* list_iterator_move_end(list_iterator_t* it) {
//[[
	assert(it != NULL);
	assert(it->list != NULL);
	it->current = it->list->head;
	list_iterator_internal_set_end(it, true);
	list_iterator_internal_check_begin(it);
	return it;
} //]]


//! Frees a list iterator
/*!
 * @param[in,out] iterator the iterator to be freed
 */
void list_iterator_free(list_iterator_t* iterator) {
//[[
	xfree(iterator);
} //]]

//! Moves a iterator one position forward
/*!
 * @param[in,out] iterator the iterator to be moved
 * @return the given list iterator
 *
 * @note
 * Please check yourself that you remain in the list!
 * Though you can hardly get a NULL ptr exception, the
 * problem will be rather that if you are behind the
 * end of a list you get back to the begin...
 */
list_iterator_t* list_iterator_next(list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	assert(iterator->current != NULL);
	iterator->current = iterator->current->next;
	list_iterator_internal_check_begin(iterator);
	list_iterator_internal_check_end(iterator);
	return iterator;
} //]]

//! Moves a iterator one position backward
/*!
 * @param[in,out] iterator the iterator to be moved
 * @return the given list iterator
 *
 * @note
 * Please check yourself that you remain in the list!
 * Though you can hardly get a NULL ptr exception, the
 * problem will be rather that if you are before the
 * begin of a list you get back to the end...
 */
list_iterator_t* list_iterator_previous(list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	assert(iterator->current != NULL);
	iterator->current = iterator->current->previous;
	list_iterator_internal_check_begin(iterator);
	list_iterator_internal_check_end(iterator);
	return iterator;
} //]]

//! Compares two list iterators
/*!
 * Returns 0 if both iterators are at the same position in
 * the same list (i.e. if the are "equal").
 *
 * @param[in] lhs left hand side, the first operator to compare
 * @param[in] rhs right hand side, the second operator to compare
 * @return 0 if both iterators are "equal"
 */
int list_iterator_cmp(const list_iterator_t* lhs, const list_iterator_t* rhs) {
//[[
	// if we have the same object they are equal
	if(rhs == lhs) {
		return 0;
	}
	// if one of them is a NULL ptr they are not equal (note we ruled alredy out that both are NULL ptr)
	if(rhs == NULL || lhs == NULL) {
		return 1;
	}
	return !(rhs->current == lhs->current && rhs->list == lhs->list);
} //]]

//! Checks wheter a given iterator is at the begin of a list
/*!
 * @param[in] iterator the iterator which must be checked
 * @return true if at begin
 */
bool list_iterator_at_begin(const list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	return iterator->at_begin;
} //]]

//! Checks wheter a given iterator is at the end of a list
/*!
 * @param[in] iterator the iterator which must be checked
 * @return true if at end
 */
bool list_iterator_at_end(const list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	return iterator->at_end;
} //]]

//! Gets a POINTER to the data from the current position
/*!
 * Read the data from the iterator at the current position
 *
 * @param[in] iterator the iterator which should be used
 * @param[out] out_size the size of the out_data pointer (must be allocated before)
 *
 * @return the out_data pointer if everything went fine, NULL otherwise
 *
 * @note if SAFE COPY is enabled you have to free the result yourself!
 *
 * @see list_iterator_get_unsafe()
 */
content_pointer_t list_iterator_get(const list_iterator_t* iterator, size_t* out_size) {
//[[
	assert(iterator != NULL);
	assert(out_size != NULL);
	if(iterator->current == iterator->list->sentinel) {
		return NULL;
	}
	*out_size = iterator->current->data_len;
	return iterator->current->data;
} //]]

//! Removes a element from the list
/*!
 * Removes the element on which the iterator points to and
 * moves after deleting the element the cursor one position
 * forward.
 *
 * \code
 * // example for deleting an entire list
 * list_t* list = list_new(NULL);
 * // insert some data into the list
 * list_iterator_t* it;
 * // no need for calling list_iterator_next(), since erase moves automatically forward
 * for(it = list_begin(list); !list_iterator_at_end(it); list_erase(it));
 * list_iterator_free(it);
 * \endcode
 *
 * @param[in] iterator the element which should be removed
 * @return true(=1) if everything went ok
 */
int list_erase(list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	if(iterator->current == iterator->list->sentinel) {
		return false;
	}
	// reset neighbor ptr elements
	if(iterator->list->head == iterator->current) {
		iterator->list->head = iterator->current->next;
	}
	iterator->current->previous->next = iterator->current->next;
	iterator->current->next->previous = iterator->current->previous;
	xfree(iterator->current->data);

	list_item_t *item = iterator->current->next;
	xfree(iterator->current);
	iterator->current = item;
	iterator->list->size--;

	list_iterator_internal_check_begin(iterator);
	list_iterator_internal_check_end(iterator);

	return true;
} //]]

//! Gets the list from a given iterator
/*!
 * @param[in] iterator the iterator whose list should be returned
 * @return NULL in case of error, otherwise a pointer to the list
 */
list_t* list_iterator_get_list(const list_iterator_t* iterator) {
//[[
	assert(iterator != NULL);
	return iterator->list;
} //]]
