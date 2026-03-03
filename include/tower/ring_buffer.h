#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdlib.h>

typedef enum
{
  DATA_TYPE_DOUBLE,
  DATA_TYPE_LONG,
} DataType;

typedef struct
{
  void *data;
  DataType data_type;
  size_t count;
  size_t head;
  size_t size;
  size_t stride;
} RingBuffer;

RingBuffer *rb_create(const size_t size, const size_t stride, const DataType data_type);

void rb_destroy(RingBuffer *rb);

void rb_push(RingBuffer *, const void *, const size_t);

const void *rb_get(const RingBuffer *, const size_t);

#endif
