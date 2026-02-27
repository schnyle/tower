#include "tower/ring_buffer.h"
#include "unity.h"

RingBuffer rb;

void setUp(void) {}

void tearDown(void) {}

void test_starts_empty(void) { TEST_ASSERT_TRUE(1 == 2); }

int main(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_starts_empty);
  return UNITY_END();
}
