// vim:encoding=latin1:foldmethod=marker:foldmarker=//[[,//]]

/**
 * @file	queue.c
 * @brief	Implementation of basic queue data structure, used for packet buffering
 * @author	Florian Weingarten <Florian.Weingarten@RWTH-Aachen.de>
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <assert.h>

#include "tools.h"
#include "queue.h"

/** Create a new queue element and add it to the end of the queue
 * @param	queue	Pointer to the queue
 * @param	data	the payload data of the new queue element (new memory is allocated
 * 			and the data is copied; after pop()ing the element, that memory has
 * 			to be free()d manually!)
 *
 * @param	datalen	the length of the payload data in bytes
 * @return	On success, 0 is returned, -1 otherwise.
 */
int queue_push(struct s_queue *queue, char *data, unsigned int datalen) {
//[[
	
	assert(queue != NULL);
	assert(data != NULL);
	assert(datalen != 0);
	
	// Get memory for new element
	struct s_queue_elem *new = malloc(sizeof(struct s_queue_elem));

	if(new == NULL) {
		print_error("malloc() failed: %s\n", strerror(errno));
		return -1;
	}

	// If the queue is empty, the new element will become the first and the last
	if(queue->len == 0) {

		// If the counter says 0, it should really BE zero!
		assert(queue->first == NULL);
		assert(queue->last  == NULL);

		queue->first = new;
		queue->last = new;

	} else {

		// If the counter says >0, the first and last should both exist
		assert(queue->first != NULL);
		assert(queue->last  != NULL);

		// Otherwise, append it to the last element
		queue->last->next = new;
	}

	// Get memory for content of new queue element
	new->data = malloc(datalen);

	if(new->data == NULL) {
		print_error("malloc() failed: %s\n", strerror(errno));
		return -1;
	}

	// Length of new queue element
	new->datalen = datalen;

	// Copy the data into the queue element
	memcpy(new->data, data, datalen);

	// New element is the last, i.e. has no successor
	new->next = NULL;

	// Update pointer to new last element
	queue->last = new;

	// We just added one element...
	queue->len++;

	return 0;

} //]]

/** Get the first element of a queue
 * @param	queue	the queue
 * @param	delete	if this is true, the element will be deleted from the queue
 * 			(and the next one becomes the first one), otherwise, it is just returned
 * 			but not deleted (in that case, the data should NOT be free()ed!)
 * @return	A copy of the first queue element is returned; Dont forget to free() the data!
 */
struct s_queue_elem queue_pop(struct s_queue *queue, int delete) {
//[[
	
	assert(queue != NULL);
	assert(queue->first != NULL);
	assert(queue->len > 0);
	
	// The return value is a copy of the queue element, not a pointer to it!
	struct s_queue_elem ret = *(queue->first);

	if(!delete) {
		return ret;
	}

	// Free the memory of the returned data
	free(queue->first);

	// Update pointer to first
	queue->first = ret.next;

	// We just removed one element...
	queue->len--;

	if(queue->len == 0) {
		queue->first = queue->last = NULL;
	}

	return ret;
} //]]

/** Free the whole queue (and all its elements)
 * @param	queue	the queue, which you want to free()
 */
void queue_free(struct s_queue *queue) {
//[[	
	assert(queue != NULL);

	struct s_queue_elem *p,*q;

	p = queue->first;

	while(p != NULL) {
		q = p;

		assert(p->data != NULL);
		assert(p->datalen != 0);

		free(p->data);
		free(p);
		p = q->next;
	}

	free(queue);

} //]]

/** Create a new queue
 * @return	On success, a pointer to the new queue is returned, NULL otherwise.
 */
struct s_queue* queue_new(void) {
//[[
	struct s_queue *ret = malloc(sizeof(struct s_queue));

	if(ret == NULL) {
		print_error("malloc() failed: %s\n", strerror(errno));
		return ret;
	}

	memset(ret, 0, sizeof(struct s_queue));

	return ret;
} //]]
