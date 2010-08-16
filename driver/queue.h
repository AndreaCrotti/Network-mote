/**
 * A generic, type safe, minimalistic and arguably stable queue.
 *
 * @date 2010-08-09
 * @author Oscar Dustmann
 *
 *
 * Usage:
 *    in any using file, you may say
 *  #include queue.h
 *  DEFILE_QUEUE(int)
 *
 *  void myFunction(void) {
 *    Queue_int* q = queue_int(NULL);
 *    q->enqueue(q,35);
 *    q->dequeue(q);
 *  }
 */
#ifndef QUEUE_H
#define QUEUE_H

#include "util.h"

#define DEFINE_QUEUE_ITEM(TYPE)                 \
/**
 * The class for queue items.
 */\
class (QueueItem_##TYPE, \
       TYPE content; \
       QueueItem_##TYPE* prev; \
       QueueItem_##TYPE* next; \
    ); \
/**
 * Destructor for queue-item objects. Do not call unless you are me.
 */\
void __queueItem_dtor(QueueItem_##TYPE* this) { \
    this->next = this->prev = NULL; \
} \
/**
 * Constructor for queue-item objects. Do not call unless you are me.
 *
 * @param prev Object on the "left" of 'this'
 * @param next Object on the "right" of 'this'
 */\
QueueItem_##TYPE* queueItem_##TYPE(QueueItem_##TYPE* this, QueueItem_##TYPE* prev, QueueItem_##TYPE* next) { \
    CTOR(this); \
    this->prev = prev; \
    if (prev) \
        prev->next = this; \
    this->next = next; \
    if (next) \
        next->prev = this; \
    return this; \
}

#define DEFINE_QUEUE(TYPE)                      \
    DEFINE_QUEUE_ITEM(TYPE)                     \
/**
 * Main queue class.
 */\
class (Queue_##TYPE, \
       QueueItem_##TYPE* head; \
       unsigned queueSize; \
       void (*enqueue)(struct Queue_##TYPE* this, TYPE const content); \
       TYPE (*dequeue)(Queue_##TYPE* this); \
       void (*clear)(Queue_##TYPE* this); \
       unsigned (*size)(Queue_##TYPE* this); \
    ); \
/**
 * Destructor for queue objects. Do not call explicitly, type DTOR(myObject) instead.
 */\
void __queue_##TYPE##_dtor(Queue_##TYPE* this) { \
    this->clear(this); \
    DTOR(this->head); \
} \
/**
 * Enqueue a value to the queue. Do not call explicitly.
 *
 * @param content The value you want to enqueue.
 */\
void __queue_##TYPE##_enqueue(Queue_##TYPE* this, TYPE content) { \
    QueueItem_##TYPE* newItem = queueItem_##TYPE(NULL,this->head,this->head->next); \
    newItem->content = content; \
    this->queueSize++; \
} \
/**
 * Dequeue a value from the queue. Do not call explicitly.
 * This will return garbage, however will not crash, if the queue is empty.
 *
 * @return The least recently enqueued value.
 */\
TYPE __queue_##TYPE##_dequeue(Queue_##TYPE* this) { \
    QueueItem_##TYPE* oldItem = this->head->prev; \
    TYPE result = oldItem->content; \
    if (this->head != oldItem) { \
        this->queueSize--; \
        this->head->prev = oldItem->prev; \
        this->head->prev->next = this->head; \
        DTOR(oldItem); \
    } \
    return result; \
} \
/**
 * Query the size of (i.e. the count of elements in) the queue.
 *
 * @return The current size of the queue.
 */\
unsigned __queue_##TYPE##_size(Queue_##TYPE* this) { \
    return this->queueSize; \
} \
/**
 * Clear the queue. I.e. dequeue everything.
 */\
void __queue_##TYPE##_clear(Queue_##TYPE* this) { \
    QueueItem_##TYPE* i = this->head->next; \
    while (i != this->head) { \
        QueueItem_##TYPE* j = i; \
        i = i->next; \
        DTOR(j); \
    } \
} \
/**
 * Create a new queue object.
 */\
Queue_##TYPE* queue_##TYPE(Queue_##TYPE* this) {\
    SETDTOR(CTOR(this)) __queue_##TYPE##_dtor; \
    this->head = queueItem_##TYPE(NULL,NULL,NULL); \
    this->head->prev = this->head; \
    this->head->next = this->head; \
    this->queueSize = 0; \
    this->enqueue = __queue_##TYPE##_enqueue; \
    this->dequeue = __queue_##TYPE##_dequeue; \
    this->clear = __queue_##TYPE##_clear; \
    this->size = __queue_##TYPE##_size; \
    return this; \
}


#endif
