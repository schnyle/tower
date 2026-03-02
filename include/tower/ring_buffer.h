#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdlib.h>

typedef struct
{
  void *data;
  size_t count;
  size_t head;
  size_t size;
  size_t stride;
} RingBuffer;

RingBuffer *rb_create(const size_t size, const size_t stride);

void rb_destroy(RingBuffer *rb);

void rb_push(RingBuffer *, const void *, const size_t);

const void *rb_get(RingBuffer *, const size_t);

#endif
