#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

template <typename T> class RingBuffer
{
public:
  explicit RingBuffer(std::size_t capacity) : buf_(capacity)
  {
    if (capacity == 0)
      throw std::invalid_argument("RingBuffer capacity must be > 0");
  }

  std::size_t size() const { return size_; }
  std::size_t capacity() const { return buf_.size(); }

  void push(const T &ele)
  {
    buf_[head_] = ele;
    head_ = (head_ + 1) % capacity();
    if (size_ < capacity())
    {
      ++size_;
    }
  }

  const T &operator[](std::size_t i) const
  {
    if (i >= size())
    {
      throw std::out_of_range("RingBuffer index out of range");
    }
    return buf_[(head_ + capacity() - size() + i) % capacity()];
  }

private:
  std::vector<T> buf_;
  std::size_t head_ = 0; // index of next write
  std::size_t size_ = 0;
};
