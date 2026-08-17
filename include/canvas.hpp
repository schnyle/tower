#pragma once

#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct Color
{
  uint8_t r, g, b;

  Color() = delete;
  constexpr Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}

  bool operator==(const Color &) const = default;

  static constexpr Color white() { return {255, 255, 255}; }
  static constexpr Color red() { return {255, 0, 0}; }
  static constexpr Color green() { return {0, 255, 0}; }
  static constexpr Color blue() { return {0, 0, 255}; }
  static constexpr Color purple() { return {128, 0, 128}; }
};

struct Cell
{
  static constexpr size_t CELL_CHAR_SIZE = 4;

  char bytes[CELL_CHAR_SIZE + 1];
  Color color;

  Cell() = delete;

  Cell(const std::string_view &s) : Cell(s, Color::white()) {}
  Cell(const std::string_view &s, const Color color) : color(color)
  {
    if (s.size() > CELL_CHAR_SIZE)
    {
      const std::string msg = std::format(
          "string {} is too wide ({} bytes) for Cell ({} bytes)", s, s.size(), CELL_CHAR_SIZE);
      throw std::invalid_argument(msg);
    }
    std::memset(bytes, 0, sizeof bytes);
    std::memcpy(bytes, s.data(), s.size());
  }

  bool operator==(const Cell &) const = default;
};

inline std::ostream &operator<<(std::ostream &os, const Cell &c) { return os << "Cell{" << c.bytes << "}"; }

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

  void copy_n(
      const size_t row,
      const size_t col,
      const std::string_view src,
      size_t src_count = std::string_view::npos,
      const std::source_location loc = std::source_location::current())
  {
    validate_row(row);
    validate_col(col);

    if (src_count == std::string_view::npos)
    {
      src_count = src.size();
    }
    src_count = std::min(src_count, src.size());

    if (col + src_count > col_count_)
    {
      throw std::out_of_range(
          std::format(
              "Canvas: col ({}) + count ({}) exceeds {} cols, called from {}:{}",
              col,
              src_count,
              col_count_,
              loc.file_name(),
              loc.line()));
    }

    for (size_t i = 0; i < src_count; ++i)
    {
      operator()(row, col + i) = Cell{src.substr(i, 1)};
    }
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
