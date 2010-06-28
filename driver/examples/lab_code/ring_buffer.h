// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

#include "defines.h"

#include <stdbool.h>

//! A ring buffer structure
/*!
 * Allows with the given functions
 * to insert/remove data into a
 * given block of data.
 */
typedef struct ring_buffer {
	//! The actual buffer/block of data
	unsigned char* buf;
	//! Points to the next position where data is inserted (always write > read % buffer size)
	unsigned char* write;
	//! Points to the next position where data is read
	unsigned char* read;
	//! Points to the end of the buffer (end = buf + buf_size)
	unsigned char* end;
	//! Length of each data element
	unsigned int data_len;
	//! the number of data elements this container can hold
	unsigned int num_data;
} ring_buffer_t;

//! Iterator for a ring buffer
/*!
 * This structure contains data which
 * is used to iterate over a given ring buffer.
 */
typedef struct ring_buffer_iterator {
	//! The ring buffer to to which this iterator is associated to
	const ring_buffer_t* ring_buf;
	//! The current position of the iterator in the ring buffer
	unsigned char* position;
} ring_buffer_iterator_t;

//******** RING BUFFER FUNCTIONS ***********/

ring_buffer_t* ring_buffer_new(const unsigned int, const unsigned int);

void ring_buffer_free(ring_buffer_t*);

ring_buffer_t* ring_buffer_copy(ring_buffer_t* dest, const ring_buffer_t* from);

ap_err_code ring_buffer_insert(ring_buffer_t*, const unsigned char*);

unsigned char* ring_buffer_read(ring_buffer_t*);

unsigned char* ring_buffer_const_read(const ring_buffer_t*);

bool ring_buffer_find(const ring_buffer_t*, const unsigned char*);

bool ring_buffer_find_and_move(ring_buffer_t*, const unsigned char*);

bool ring_buffer_remove(ring_buffer_t*, const unsigned char*);

//******** RING BUFFER ITERATOR FUNCTIONS ***********/

ring_buffer_iterator_t* ring_buffer_iterator_new(const ring_buffer_t*);

void ring_buffer_iterator_free(ring_buffer_iterator_t*);

ap_err_code ring_buffer_iterator_reset(ring_buffer_iterator_t*);

unsigned char* ring_buffer_iterator_next(ring_buffer_iterator_t*);

unsigned char* ring_buffer_iterator_get(const ring_buffer_iterator_t*);

bool ring_buffer_iterator_at_end(const ring_buffer_iterator_t*);

#endif /*RING_BUFFER_H_*/
