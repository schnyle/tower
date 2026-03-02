#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tower/log.h"
#include "tower/ring_buffer.h"

RingBuffer *rb_create(const size_t size, const size_t stride)
{
  if (size == 0 || stride == 0)
  {
    LOG_ERROR("size and stride parameters must be nonzero");
    return NULL;
  }

  RingBuffer *rb = malloc(sizeof(RingBuffer));
  if (!rb)
  {
    LOG_ERROR("failed to allocate RingBuffer");
    return NULL;
  }

  rb->data = malloc(size * stride);
  if (!rb->data)
  {
    free(rb);
    LOG_ERROR("failed to allocate RingBuffer data");
    return NULL;
  }

  rb->count = 0;
  rb->head = size - 1;
  rb->size = size;
  rb->stride = stride;

  return rb;
}

void rb_destroy(RingBuffer *rb)
{
  free(rb->data);
  free(rb);
}

void rb_push(RingBuffer *rb, const void *val, const size_t len)
{
  if (len != rb->stride)
  {
    LOG_ERROR("attempted to push object of size %lu to RingBuffer with stride %lu, skipping\n", len, rb->stride);
    return;
  };

  void *dest = (char *)rb->data + (++rb->head % rb->size) * rb->stride;
  memcpy(dest, val, rb->stride);
  rb->count = rb->count == rb->size ? rb->size : rb->count + 1;
  rb->head %= rb->size;
};

const void *rb_get(RingBuffer *rb, const size_t i)
{
  if (i >= rb->count)
  {
    LOG_ERROR("attempted to access element at index %d of RingBuffer with %d elements", i, rb->count);
    return NULL;
  }

  const size_t tail = rb->count == rb->size ? (rb->head + 1) % rb->size : 0;
  return rb->data + tail + i;
};
