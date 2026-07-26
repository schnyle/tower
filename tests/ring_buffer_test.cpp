#include <gtest/gtest.h>
#include <optional>
#include <stdexcept>

#include "ring_buffer.hpp"

// basic

TEST(RingBuffer, CanBeConstructedWithCapacity) { RingBuffer<int> rb(4); }

TEST(RingBuffer, ThrowsOnZeroCapacity) { EXPECT_THROW(RingBuffer<int>(0), std::invalid_argument); }

// initial state

TEST(RingBuffer, NewBufferIsEmpty)
{
  RingBuffer<int> rb(4);
  EXPECT_EQ(rb.size(), 0u);
}

TEST(RingBuffer, NewBufferReportsCapacity)
{
  RingBuffer<int> rb(4);
  EXPECT_EQ(rb.capacity(), 4u);
}

TEST(RingBuffer, NewBufferNewestIsNullopt)
{
  RingBuffer<int> rb(4);
  EXPECT_EQ(rb.newest(), std::nullopt);
}

TEST(RingBuffer, PushBelowCapacityIncreasesSize)
{
  RingBuffer<int> rb(4);
  rb.push(7);
  EXPECT_EQ(rb.size(), 1u);
}

TEST(RingBuffer, PushAtCapacityMaintainsSize)
{
  RingBuffer<int> rb(4);
  for (int i = 1; i <= 5; ++i)
  {
    rb.push(i);
  }
  EXPECT_EQ(rb.size(), 4u);
}

TEST(RingBuffer, PushedValueIsReadable)
{
  RingBuffer<int> rb(4);
  rb.push(7);
  EXPECT_EQ(rb[0], 7);
}

TEST(RingBuffer, PushPastCapacityOverwritesOldest)
{
  RingBuffer<int> rb(4);
  rb.push(1);
  rb.push(2);
  rb.push(3);
  rb.push(4); // size == capacity
  rb.push(5);

  // temporal ordering is now [2, 3, 4, 5]
  EXPECT_EQ(rb[0], 2);
  EXPECT_EQ(rb[3], 5);
}

TEST(RingBuffer, NewestReturnsLastPushedWhenBelowCapacity)
{
  RingBuffer<int> rb(4);
  rb.push(1);
  rb.push(2);
  rb.push(3);

  EXPECT_EQ(rb.newest(), 3);
}

TEST(RingBuffer, NewestReturnsLastPushedWhenAtCapacity)
{
  RingBuffer<int> rb(4);
  rb.push(1);
  rb.push(2);
  rb.push(3);
  rb.push(4); // size == capacity
  rb.push(5);

  EXPECT_EQ(rb.newest(), 5);
}

TEST(RingBuffer, ThrowsOnOutOfRangeAccess)
{
  RingBuffer<int> rb(4);
  rb.push(1);
  rb.push(2);
  EXPECT_THROW(rb[2], std::out_of_range); // check when size < capacity
  EXPECT_NO_THROW(rb[1]);

  rb.push(3);
  rb.push(4);
  rb.push(5);
  EXPECT_THROW(rb[4], std::out_of_range); // check when size == capacity
  EXPECT_NO_THROW(rb[3]);
}

// TODO
//   - capacity 0 should not work
