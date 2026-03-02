#include "tower/ring_buffer.h"
#include "unity.h"

void setUp(void) {}

void tearDown(void) {}

void test_initialization(void)
{
  const size_t size = 32;
  const size_t stride = 8;
  RingBuffer *rb = rb_create(size, stride);

  TEST_ASSERT_EQUAL_INT(0, rb->count);
  TEST_ASSERT_EQUAL_INT(size - 1, rb->head);
  TEST_ASSERT_EQUAL_INT(size, rb->size);
  TEST_ASSERT_EQUAL_INT(stride, rb->stride);

  rb_destroy(rb);
}

void test_push_increases_size(void)
{
  const size_t size = 32;
  const size_t stride = sizeof(char);
  RingBuffer *rb = rb_create(size, stride);

  const char x = 'h';
  rb_push(rb, &x, sizeof(x));

  TEST_ASSERT_EQUAL_INT(1, rb->count);
  TEST_ASSERT_EQUAL_INT(0, rb->head);

  rb_destroy(rb);
}

void test_push_incorrect_size(void)
{
  const size_t size = 32;
  const size_t stride = sizeof(char);
  RingBuffer *rb = rb_create(size, stride);

  const long long x = 0LL;
  rb_push(rb, &x, sizeof(x));

  TEST_ASSERT_EQUAL_INT(0, rb->count);
  TEST_ASSERT_EQUAL_INT(rb->size - 1, rb->head);

  rb_destroy(rb);
}

void test_ring_behavior(void)
{
  const char f = 'f';
  const char k = 'k';
  const char z = 'z';
  const char a = 'a';
  const size_t size = 3;
  const size_t stride = sizeof(char);
  RingBuffer *rb = rb_create(size, stride);

  rb_push(rb, &f, sizeof(f));
  TEST_ASSERT_EQUAL_INT(1, rb->count);
  TEST_ASSERT_EQUAL_INT(0, rb->head);
  TEST_ASSERT_EQUAL_CHAR(f, *((char *)rb->data + 0));

  rb_push(rb, &k, sizeof(k));
  TEST_ASSERT_EQUAL_INT(2, rb->count);
  TEST_ASSERT_EQUAL_INT(1, rb->head);
  TEST_ASSERT_EQUAL_CHAR(k, *((char *)rb->data + 1));

  rb_push(rb, &z, sizeof(z));
  TEST_ASSERT_EQUAL_INT(3, rb->count);
  TEST_ASSERT_EQUAL_INT(2, rb->head);
  TEST_ASSERT_EQUAL_CHAR(z, *((char *)rb->data + 2));

  rb_push(rb, &a, sizeof(a));
  TEST_ASSERT_EQUAL_INT(3, rb->count);
  TEST_ASSERT_EQUAL_INT(0, rb->head);
  TEST_ASSERT_EQUAL_CHAR(a, *((char *)rb->data + 0));

  rb_destroy(rb);
}

void test_get_element_by_index(void)
{
  const char g = 'g';
  const char a = 'a';
  const char x = 'x';
  const size_t size = 3;
  const size_t stride = sizeof(char);
  RingBuffer *rb = rb_create(size, stride);

  rb_push(rb, &g, sizeof(g));
  TEST_ASSERT_EQUAL_CHAR(g, *(char *)rb_get(rb, 0));
  TEST_ASSERT_NULL(rb_get(rb, 1));
  TEST_ASSERT_NULL(rb_get(rb, 2));

  rb_push(rb, &a, sizeof(a));
  TEST_ASSERT_EQUAL_CHAR(g, *(char *)rb_get(rb, 0));
  TEST_ASSERT_EQUAL_CHAR(a, *(char *)rb_get(rb, 1));
  TEST_ASSERT_NULL(rb_get(rb, 2));

  rb_push(rb, &x, sizeof(x));
  TEST_ASSERT_EQUAL_CHAR(g, *(char *)rb_get(rb, 0));
  TEST_ASSERT_EQUAL_CHAR(a, *(char *)rb_get(rb, 1));
  TEST_ASSERT_EQUAL_CHAR(x, *(char *)rb_get(rb, 2));

  rb_destroy(rb);
}

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_initialization);
  RUN_TEST(test_push_increases_size);
  RUN_TEST(test_push_incorrect_size);
  RUN_TEST(test_ring_behavior);
  RUN_TEST(test_get_element_by_index);
  return UNITY_END();
}
