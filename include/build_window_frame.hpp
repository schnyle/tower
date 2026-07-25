#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr int NAME_OFFSET = 2;

inline void validate_inputs(
    const std::vector<std::vector<std::string>> &dest,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count)
{
  if (row_count == 0)
  {
    throw std::invalid_argument("build_bar_plot: rows must be > 0");
  }

  if (col_count == 0)
  {
    throw std::invalid_argument("build_bar_plot: cols must be > 0");
  }

  if (dest.size() < row_offset + row_count)
  {
    const auto msg = std::format(
        "build_bar_plot: row_offset ({}) + row_count ({}) exceeds dest rows ({})", row_offset, row_count, dest.size());
    throw std::invalid_argument(msg);
  }

  if (dest[row_offset].size() < col_offset + col_count)
  {
    const auto msg = std::format(
        "build_bar_plot: col_offset ({}) + col_offset ({}) exceeds dest cols ({})",
        col_offset,
        col_count,
        dest[row_offset].size());
    throw std::invalid_argument(msg);
  }
}

inline void build_window_frame(
    std::vector<std::vector<std::string>> &dest,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count,
    const std::string &name = "")
{
  validate_inputs(dest, row_offset, col_offset, row_count, col_count);

  // top & bottom
  for (size_t col = col_offset; col < col_offset + col_count; ++col)
  {
    dest[row_offset][col] = "─";
    dest[row_offset + row_count - 1][col] = "─";
  }

  // left & right
  for (size_t row = row_offset; row < row_offset + row_count; ++row)
  {
    dest[row][col_offset] = "│";
    dest[row][col_offset + col_count - 1] = "│";
  }

  // corners
  dest[row_offset][col_offset] = "┌";
  dest[row_offset + row_count - 1][col_offset] = "└";
  dest[row_offset][col_offset + col_count - 1] = "┐";
  dest[row_offset + row_count - 1][col_offset + col_count - 1] = "┘";

  // title
  auto &header = dest[row_offset];
  if (name != "" && col_count > name.size() + 3)
  {
    header[col_offset + NAME_OFFSET] = " ";
    std::copy(name.begin(), name.end(), header.begin() + col_offset + NAME_OFFSET + 1);
    header[col_offset + NAME_OFFSET + name.size() + 1] = " ";
  }
}
