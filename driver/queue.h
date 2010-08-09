#ifndef QUEUE_H
#define QUEUE_H

#include "util.h"

#define DEFINE_QUEUE_ITEM(TYPE) \
class (QueueItem_##TYPE, \
  TYPE content; \
  QueueItem_##TYPE* prev; \
  QueueItem_##TYPE* next; \
); \
void __queueItem_dtor(QueueItem_##TYPE* this) { \
  this->next = this->prev = NULL; \
} \
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

#define DEFINE_QUEUE(TYPE) \
DEFINE_QUEUE_ITEM(TYPE) \
class (Queue_##TYPE, \
  QueueItem_##TYPE* head; \
  unsigned queueSize; \
  void (*enqueue)(struct Queue_##TYPE* this, TYPE const content); \
  TYPE (*dequeue)(Queue_##TYPE* this); \
  void (*clear)(Queue_##TYPE* this); \
  unsigned (*size)(Queue_##TYPE* this); \
); \
void __queue_##TYPE##_dtor(Queue_##TYPE* this) { \
  this->clear(this); \
  DTOR(this->head); \
} \
void __queue_##TYPE##_enqueue(Queue_##TYPE* this, TYPE content) { \
  QueueItem_##TYPE* newItem = queueItem_##TYPE(NULL,this->head,this->head->next); \
  newItem->content = content; \
  this->queueSize++; \
} \
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
unsigned __queue_##TYPE##_size(Queue_##TYPE* this) { \
  return this->queueSize; \
} \
void __queue_##TYPE##_clear(Queue_##TYPE* this) { \
  QueueItem_##TYPE* i = this->head->next; \
  while (i != this->head) { \
    QueueItem_##TYPE* j = i; \
    i = i->next; \
    DTOR(j); \
  } \
} \
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
