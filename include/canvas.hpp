#pragma once

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

struct Cell
{
  std::string value;
  bool operator==(const Cell &) const = default;
};

inline std::ostream &operator<<(std::ostream &os, const Cell &c) { return os << "Cell{" << c.value << "}"; }

class Canvas
{
public:
  Canvas(const size_t row_count, const size_t col_count)
      : row_count_(row_count), col_count_(col_count), buf_(row_count * col_count, Cell{" "})
  {
    if (row_count == 0 || col_count == 0)
    {
      const std::string msg = std::format("row count ({}) and col count ({}) must be non-zero", row_count, col_count);
      throw std::invalid_argument(msg);
    }
  }

  const size_t &row_count() const { return row_count_; }
  const size_t &col_count() const { return col_count_; }

  Cell &operator()(const size_t row, const size_t col)
  {
    validate_row(row);
    validate_col(col);
    return buf_[row * col_count() + col];
  }

  std::vector<Cell>::iterator row_begin(const size_t row)
  {
    validate_row(row);
    return buf_.begin() + (row * col_count());
  }
  std::vector<Cell>::iterator row_end(const size_t row)
  {
    validate_row(row);
    return buf_.begin() + ((row + 1) * col_count());
  }

private:
  const size_t row_count_;
  const size_t col_count_;
  std::vector<Cell> buf_;

  void validate_row(const size_t row) const
  {
    if (row >= row_count_)
    {
      throw std::out_of_range(std::format("Canvas: row ({}) out of range for {} rows", row, row_count_));
    }
  }

  void validate_col(const size_t col) const
  {
    if (col >= col_count_)
    {
      throw std::out_of_range(std::format("Canvas: col ({}) out of range for {} cols", col, col_count_));
    }
  }
};
