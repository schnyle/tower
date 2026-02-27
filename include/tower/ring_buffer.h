#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#define RING_BUFFER_SIZE 1024

typedef struct
{
  void *data[RING_BUFFER_SIZE];
  int head;
  int count;
} RingBuffer;

void rb_init(RingBuffer *);

void rb_push(RingBuffer *, void *);

#endif
