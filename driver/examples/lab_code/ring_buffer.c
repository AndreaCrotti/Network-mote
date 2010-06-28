// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]
/*
 * Copyright (c) 2009, Distributed Systems Group, RWTH Aachen
 * All rights reserved.
 */

/**
 * @file	ring_buffer.c
 * @brief	Ring-Buffer data-structure, accessor and iterator-methods
 */

#include "ring_buffer.h"
#include "defines.h"
#include "xmalloc.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

//! Moves a pointer one position forward
/*!
 * This function advances the given pointer by one position and checks
 * whether the new location is in the ring buffer. If not, the pointer
 * will be set to the beginning of the ring buffer.
 *
 * @param[in] buf the ring buffer whose borders should be taken
 * @param[in,out] ptr the pointer which should be incremented
 */
inline static void ring_buffer_move_forward(const ring_buffer_t* buf, unsigned char** ptr) {
//[[
	if(*ptr == buf->end) {
		*ptr = buf->buf;
	} else {
		*ptr += buf->data_len;
	}
} //]]

//! Moves a pointer one position backward
/*!
 * @see ring_buffer_move_forward
 * @param[in] buf the ring buffer whose borders should be considered
 * @param[in,out] ptr the pointer which should be decremented
 */
static void ring_buffer_move_backward(const ring_buffer_t* buf, unsigned char** ptr) {
//[[
	if(*ptr == buf->buf) {
		*ptr = buf->end;
	} else {
		*ptr -= buf->data_len;
	}
} //]]

//! Searches for a data element in the ring buffer
/*!
 * Searches the entire ring buffer starting from the current read
 * element for a given element.
 *
 * @param[in] buf the ring buffer to be searched
 * @param[in] to_find the element which should be found
 * @return NULL if the element is not in the ring buffer otherwise a pointer to the element in the ring buffer
 */
inline static unsigned char* ring_buffer_find_element(const ring_buffer_t* buf, const unsigned char* to_find) {
//[[
	assert(buf != NULL);
	assert(buf->buf != NULL);
	assert(to_find != NULL);
	unsigned char* ptr = buf->read;
	do {
		if(memcmp(ptr, to_find, buf->data_len) == 0) {
			return ptr;
		}
		ring_buffer_move_forward(buf, &ptr);
	} while(ptr != buf->read);
	return NULL;
} //]]

//! Creates a new ring buffer
/*!
 * A new ring buffer is created, containing num_data elements of size data_len. The data structure
 * is allocated (malloced) and initialized.
 *
 * @param[in] num_data the number of data items this container/buffer should contain of size data_len
 * @param[in] data_len size of each element in the buffer in bytes
 *
 * @return new allocated ring buffer
 *
 * @note Use ring_buffer_free to free the returned pointer after usage
 */
ring_buffer_t* ring_buffer_new(const unsigned int num_data, const unsigned int data_len) {
//[[
	assert(num_data > 0);
	assert(data_len > 0);
	ring_buffer_t* r = xmalloc(sizeof(ring_buffer_t));
	r->data_len = data_len;
	r->num_data = num_data;
	r->buf = xmalloc(num_data * data_len);
	r->write = r->read = r->buf;
	r->end = r->buf + ((num_data - 1) * data_len);
	memset(r->buf, 0, data_len * num_data);
	return r;
} //]]

//! Frees a given ring buffer
/*!
 * Frees a given ring buffer if not NULL as an argument is supplied. If NULL is supplied nothing happens.
 * @param[in,out] buf the ring buffer to be freed or NULL
 */
void ring_buffer_free(ring_buffer_t* buf) {
//[[
	if(buf) {
		if(buf->buf)
			xfree(buf->buf);
		xfree(buf);
	}
	return;
} //]]

//! Copys elements from one ring buffer to another one
/*!
 * Copys all elements from the source to the destination. If
 * there are more elements in the source than this function
 * stops when destiation is full. If source is smaller all
 * elements from source are inserted into destiation at the
 * beginning of the ring buffer.
 *
 * @note The data length of both buffers need to be the same!
 *
 * @param[in] destination where the content should be put into
 * @param[in] source where the content is stored which should be copied
 *
 * @return pointer to destination
 */
ring_buffer_t* ring_buffer_copy(ring_buffer_t* destination, const ring_buffer_t* source) {
//[[
	assert(destination != NULL);
	assert(destination != NULL);
	assert(destination->data_len == source->data_len);
	unsigned int num_elements_to_copy = (destination->num_data > source->num_data) ? source->num_data : destination->num_data;
	memcpy(destination->buf, source->buf, destination->data_len * (num_elements_to_copy));
	destination->read = destination->buf + (source->read - source->buf);
	destination->write = destination->buf + (source->write - source->buf);
	return destination;
} //]]

//! Inserts a new element into the ring buffer and advances one position
/*!
 * Inserts and overwrites the ringbuffer at the current position and advances one position.
 * If the read position is at the same position as the new write position is, the read
 * position will also be increased by one. The length of the data is given by the ring buffer.
 *
 * @param[in,out] buf the ring buffer in which the data should be inserted
 * @param[in] data the data which is copied into the ringbuffer
 *
 * @see ring_buffer_new()
 */
ap_err_code ring_buffer_insert(ring_buffer_t* buf, const unsigned char* data) {
//[[
	assert(buf != NULL);
	assert(buf->buf != NULL);
	assert(data != NULL);
	memcpy(buf->write, data, buf->data_len);
	ring_buffer_move_forward(buf, &buf->write);
	if(buf->write == buf->read) {
		ring_buffer_move_forward(buf, &buf->read);
	}
	return AP_ERR_SUCCESS;
} //]]

//! Reads a element from the ring buffer and advances one position
/*!
 * Tried to read data from the ring buffer. If no data is available NULL is returned.
 * After reading, the position of the read pointer is increased if possible (i.e. if the
 * write pointer is not directy in front of the read pointer).
 *
 * @param[in] buf the buffer of which data should be retrieved
 *
 * @return NULL or the element in the ringbuffer
 *
 * @see ring_buffer_const_read() ring_buffer_insert()
 *
 * @note the size of returning element is given by the constructor function ring_buffer_create()
 */
unsigned char* ring_buffer_read(ring_buffer_t* buf) {
//[[
	assert(buf != NULL);
	unsigned char* r = ring_buffer_const_read(buf);
	if(r != NULL) {
		ring_buffer_move_forward(buf, &buf->read);
	}
	return r;
} //]]

//! Reads a element from the ring buffer
/*!
 * This function does the same as ring_buffer_read() but does not advance the read pointer.
 * So if you want to read a data element multiple times use this function.
 *
 * @param[in] buf the ring buffer in the which element is
 *
 * @return NULL or the element in the ringbuffer
 */
unsigned char* ring_buffer_const_read(const ring_buffer_t* buf) {
//[[
	if(buf->read == buf->write) {
		return NULL;
	}
	return buf->read;
} //]]

//! Searches a element in the ring buffer
/*!
 * Searches a element in the ring buffer by searching from the read pointer upto the write pointer
 * (i.e. all elements that have not been read yet but have been written into the buffer).
 *
 * @param[in] buf the ring buffer in which the element should be searched
 * @param[in] to_find the element which should be search for
 *
 * @note the size of to_find is given by the constructor function ring_buffer_create()
 *
 * @return true if the element has been found
 */
inline bool ring_buffer_find(const ring_buffer_t* buf, const unsigned char* to_find) {
//[[
	assert(buf != NULL);
	assert(buf->buf != NULL);
	assert(to_find != NULL);
	return ring_buffer_find_element(buf, to_find) != NULL;
} //]]

//! Searches for a given element and moves the read pointer to this element
/*!
 * Searches the ring buffer for a given element and if it is found the read
 * pointer will be set to that position, so that the next call of
 * ring_buffer_const_read() will return the searched element. If the element
 * is not found no operation is performed and false is returned.
 *
 * @see ring_buffer_find()
 * @param[in,out] buf the ring buffer to be searched
 * @param[in] to_find the element which shall be found
 * @return true if element is found
 */
inline bool ring_buffer_find_and_move(ring_buffer_t* buf, const unsigned char* to_find) {
//[[
	assert(buf != NULL);
	assert(buf->buf != NULL);
	assert(to_find != NULL);
	unsigned char* ptr = ring_buffer_find_element(buf, to_find);
	if(!ptr) {
		return false;
	} else {
		buf->read = ptr;
		return true;
	}
} //]]

//! Removes a element from the ring buffer
/*!
 * Searches for a element in the ring buffer and if it exists it
 * is removed. On the position where the element has been, the
 * element before the write pointer (i.e. the last inserted
 * element) is placed. Therefore the write pointer is also
 * decremented after a successful operation. If the element is
 * not found no operation is performed.
 *
 * @param[in,out] buf the ring buffer to be searched
 * @param[in] to_remove the element which shall be removed
 * @return true if the element has been removed, otherwise false (if it is not in the ring buffer)
 *
 * @see ring_buffer_find()
 */
bool ring_buffer_remove(ring_buffer_t* buf, const unsigned char* to_remove) {
//[[
	unsigned char* to_rm_ptr = ring_buffer_find_element(buf, to_remove);
	if(to_rm_ptr == NULL) {
		return false;
	}
	unsigned char* before_write_ptr = buf->write;
	ring_buffer_move_backward(buf, &before_write_ptr);
	if(to_rm_ptr != before_write_ptr) {
		memcpy(to_rm_ptr, before_write_ptr, buf->data_len);
	}
	memset(before_write_ptr, 0, buf->data_len);
	buf->write = before_write_ptr;
	return true;
} //]]


/////////////////////////////////////////////////////////////////////////
//                 RING BUFFER ITERATOR
/////////////////////////////////////////////////////////////////////////

//! Creates a new ring buffer iterator
/*!
 * A ring buffer iterator moves along the ring buffer from the read pointer up
 * to the write pointer (so all along the content).
 *
 * @note You get a new malloced iterator, use ring_buffer_iterator_free() to free this instance
 *
 * @param[in] buf the ring buffer for which a iterator shall be created
 * @return the newly created iterator
 *
 * @see ring_buffer_iterator_free(), ring_buffer_iterator_reset()
 *
 * @remark You have to keep your iterator consistent with the ring buffer yourself (i.e. use ring_buffer_iterator_reset())
 */
ring_buffer_iterator_t* ring_buffer_iterator_new(const ring_buffer_t* buf) {
//[[
	assert(buf != NULL);
	ring_buffer_iterator_t* retval = xmalloc(sizeof(ring_buffer_iterator_t));
	retval->ring_buf = buf;
	retval->position = buf->read;
	return retval;
} //]]

//! Free a created ring buffer iterator
/*!
 * Frees a ring buffer iterator but does not perform any operations
 * on the assigned ring buffer. If NULL is passed no operation is
 * performed.
 *
 * @see ring_buffer_iterator_new()
 * @param[in,out] buf_it the iterator which shall be freed or NULL
 */
inline void ring_buffer_iterator_free(ring_buffer_iterator_t* buf_it) {
//[[
	xfree(buf_it);
} //]]

//! Resets a ring buffer iterator
/*!
 * The given ring buffer iterator will point to the read pointer of
 * the assigned ring buffer. Use this function if you have modified
 * the associated ring buffer (i.e. inserted / removed data) in
 * order to keep a consistent ring buffer iterator.
 *
 * @param[in,out] buf_it the iterator which shall be reseted
 * @return AP_ERR_SUCCESS
 */
ap_err_code ring_buffer_iterator_reset(ring_buffer_iterator_t* buf_it) {
//[[
	assert(buf_it != NULL);
	buf_it->position = buf_it->ring_buf->read;
	return AP_ERR_SUCCESS;
} //]]

//! Returns the current element to which the iterator points
/*!
 * Returns the element to which the iterator points currently, or NULL
 * if the iterator has reached the end. This function does not check
 * whether the iterator is out of bounds!
 *
 * @param[in] buf_it the iterator whose position/data should be returned
 * @return position where the pointer currently points to or NULL
 */
inline unsigned char* ring_buffer_iterator_get(const ring_buffer_iterator_t* buf_it) {
//[[
	assert(buf_it != NULL);
	return (buf_it->ring_buf->write == buf_it->ring_buf->read) ? NULL : buf_it->position;
} //]]

//! Moves the ring buffer iterator one position forward
/*!
 * Moves the iterator one position forward and returns the
 * element to which the iterator points after having moved
 * forward. Returns NULL if the ring buffer iterator has
 * reached the end (i.e. the write pointer)
 *
 * @param[in,out] buf_it the ring buffer iterator which shall be moved
 * @return NULL if the iterator has reached the end or the element to which the iterator points after moving
 */
unsigned char* ring_buffer_iterator_next(ring_buffer_iterator_t* buf_it) {
//[[
	assert(buf_it != NULL);
	assert(buf_it->ring_buf != NULL);
	if(buf_it->position != buf_it->ring_buf->write) {
		ring_buffer_move_forward(buf_it->ring_buf, &buf_it->position);
		return ring_buffer_iterator_get(buf_it);
	} else {
		return NULL;
	}
} //]]
//
//! Checks wheter the iterator has reached the end
/*!
 * Checks wheter the iterator has reached the end (i.e. the write pointer),
 * but does NOT check wheter it is out of bounds!
 *
 * @param[in] buf_it the ring buffer iterator which shall be checked
 * @return true if it has reached the end
 *
 * @see ring_buffer_iterator_get()
 */
bool ring_buffer_iterator_at_end(const ring_buffer_iterator_t* buf_it) {
//[[
	assert(buf_it != NULL);
	return buf_it->position == buf_it->ring_buf->write;
} //]]
