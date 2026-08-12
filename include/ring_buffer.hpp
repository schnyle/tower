#pragma once

#include <cstddef>
#include <optional>
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

  const T &operator[](std::size_t i) const
  {
    if (i >= size())
    {
      throw std::out_of_range("RingBuffer index out of range");
    }
    return buf_[(head_ + capacity() - size() + i) % capacity()];
  }

  std::optional<T> newest() const
  {
    if (size() == 0)
    {
      return std::nullopt;
    }
    return buf_[(head_ + capacity() - 1) % capacity()];
  }

  void push(const T &ele)
  {
    buf_[head_] = ele;
    head_ = (head_ + 1) % capacity();
    if (size_ < capacity())
    {
      ++size_;
    }
  }

  void repeat_last()
  {
    if (const auto val = newest())
    {
      push(*val);
    }
  }

  void clear()
  {
    head_ = 0;
    size_ = 0;
  }

private:
  std::vector<T> buf_;
  std::size_t head_ = 0; // index of next write
  std::size_t size_ = 0;
};
