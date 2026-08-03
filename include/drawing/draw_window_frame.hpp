#pragma once

#include <format>
#include <stdexcept>
#include <string>

#include "canvas.hpp"

static constexpr int NAME_OFFSET = 2;

inline void validate_inputs(
    Canvas &dest,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count)
{
  if (row_count == 0)
  {
    throw std::invalid_argument("draw_window_frame: rows must be > 0");
  }

  if (col_count == 0)
  {
    throw std::invalid_argument("draw_window_frame: cols must be > 0");
  }

  if (dest.row_count() < row_offset + row_count)
  {
    const auto msg = std::format(
        "draw_window_frame: row_offset ({}) + row_count ({}) exceeds dest rows ({})",
        row_offset,
        row_count,
        dest.row_count());
    throw std::invalid_argument(msg);
  }

  if (dest.col_count() < col_offset + col_count)
  {
    const auto msg = std::format(
        "draw_window_frame: col_offset ({}) + col_offset ({}) exceeds dest cols ({})",
        col_offset,
        col_count,
        dest.col_count());
    throw std::invalid_argument(msg);
  }
}

inline void draw_window_frame(
    Canvas &canvas,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count,
    const std::string &name = "")
{
  validate_inputs(canvas, row_offset, col_offset, row_count, col_count);

  // top & bottom
  for (size_t col = col_offset; col < col_offset + col_count; ++col)
  {
    canvas(row_offset, col) = Cell{"─"};
    canvas(row_offset + row_count - 1, col) = Cell{"─"};
  }

  // left & right
  for (size_t row = row_offset; row < row_offset + row_count; ++row)
  {
    canvas(row, col_offset) = Cell{"│"};
    canvas(row, col_offset + col_count - 1) = Cell{"│"};
  }

  // corners
  canvas(row_offset, col_offset) = Cell{"┌"};
  canvas(row_offset + row_count - 1, col_offset) = Cell{"└"};
  canvas(row_offset, col_offset + col_count - 1) = Cell{"┐"};
  canvas(row_offset + row_count - 1, col_offset + col_count - 1) = Cell{"┘"};

  // title
  if (name != "" && col_count > name.size() + 3)
  {
    canvas(row_offset, col_offset + NAME_OFFSET) = Cell{" "};
    for (size_t i = 0; i < name.size(); ++i)
    {
      canvas(row_offset, col_offset + NAME_OFFSET + i + 1) = Cell{std::string(1, name[i])};
    }
    canvas(row_offset, col_offset + NAME_OFFSET + name.size() + 1) = Cell{" "};
  }
}
