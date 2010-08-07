#ifndef QUEUE_H
#define QUEUE_H

#include "util.h"

#define DEFINE_QUEUE_ITEM(TYPE) \
class (QueueItem_##TYPE, \
  TYPE content; \
  QueueItem_##TYPE* next; \
); \
QueueItem_##TYPE* queueItem_##TYPE(QueueItem_##TYPE* this, QueueItem_##TYPE* next) { \
  CTOR(this); \
  this->next = next; \
  return this; \
}

#define DEFINE_QUEUE(TYPE) \
DEFINE_QUEUE_ITEM(TYPE) \
class (Queue_##TYPE, \
  QueueItem_##TYPE* head; \
  QueueItem_##TYPE* tail; \
  unsigned queueSize; \
  void (*enqueue)(struct Queue_##TYPE* this, TYPE const content); \
  TYPE (*dequeue)(Queue_##TYPE* this); \
  void (*clear)(Queue_##TYPE* this); \
  unsigned (*size)(Queue_##TYPE* this); \
); \
void __queue_##TYPE##_dtor(Queue_##TYPE* this) { \
  this->clear(this); \
  DTOR(this->head); \
  this->tail = NULL; \
} \
void __queue_##TYPE##_enqueue(Queue_##TYPE* this, TYPE content) { \
  this->tail->next = queueItem_##TYPE(NULL,this->tail->next); \
  this->tail->content = content; \
  this->queueSize++; \
} \
TYPE __queue_##TYPE##_dequeue(Queue_##TYPE* this) { \
  TYPE result = this->head->content; \
  if (this->head != this->tail) { \
    this->queueSize--; \
    QueueItem_##TYPE* oldhead = this->head; \
    this->head = this->head->next; \
    DTOR(oldhead); \
  } \
  return result; \
} \
unsigned __queue_##TYPE##_size(Queue_##TYPE* this) { \
  return this->queueSize; \
} \
void __queue_##TYPE##_clear(Queue_##TYPE* this) { \
  QueueItem_##TYPE* i = this->head; \
  while (i->next != i) { \
    QueueItem_##TYPE* j = i; \
    i = i->next; \
    DTOR(j); \
  } \
} \
Queue_##TYPE* queue_##TYPE(Queue_##TYPE* this) {\
  SETDTOR(CTOR(this)) __queue_##TYPE##_dtor; \
  this->head = this->tail = queueItem_##TYPE(NULL,NULL); \
  this->head->next = this->head; \
  this->queueSize = 0; \
  this->enqueue = __queue_##TYPE##_enqueue; \
  this->dequeue = __queue_##TYPE##_dequeue; \
  this->clear = __queue_##TYPE##_clear; \
  this->size = __queue_##TYPE##_size; \
  return this; \
}


#endif
