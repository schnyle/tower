#pragma once

#include <format>
#include <stdexcept>
#include <string>

#include "canvas.hpp"
#include "windows/window.hpp"

static constexpr int NAME_OFFSET = 2;

inline void validate_inputs(Canvas &dest, const Rect rect)
{
  if (rect.row_count == 0)
  {
    throw std::invalid_argument("draw_window_frame: rows must be > 0");
  }

  if (rect.col_count == 0)
  {
    throw std::invalid_argument("draw_window_frame: cols must be > 0");
  }

  if (dest.row_count() < rect.row_offset + rect.row_count)
  {
    const auto msg = std::format(
        "draw_window_frame: row_offset ({}) + row_count ({}) exceeds dest rows ({})",
        rect.row_offset,
        rect.row_count,
        dest.row_count());
    throw std::invalid_argument(msg);
  }

  if (dest.col_count() < rect.col_offset + rect.col_count)
  {
    const auto msg = std::format(
        "draw_window_frame: col_offset ({}) + col_offset ({}) exceeds dest cols ({})",
        rect.col_offset,
        rect.col_count,
        dest.col_count());
    throw std::invalid_argument(msg);
  }
}

inline void draw_window_frame(Canvas &canvas, const Rect rect, const std::string &name = "")
{
  validate_inputs(canvas, rect);

  // top & bottom
  for (size_t col = rect.col_offset; col < rect.col_offset + rect.col_count; ++col)
  {
    canvas(rect.row_offset, col) = Cell{"─"};
    canvas(rect.row_offset + rect.row_count - 1, col) = Cell{"─"};
  }

  // left & right
  for (size_t row = rect.row_offset; row < rect.row_offset + rect.row_count; ++row)
  {
    canvas(row, rect.col_offset) = Cell{"│"};
    canvas(row, rect.col_offset + rect.col_count - 1) = Cell{"│"};
  }

  // corners
  canvas(rect.row_offset, rect.col_offset) = Cell{"┌"};
  canvas(rect.row_offset + rect.row_count - 1, rect.col_offset) = Cell{"└"};
  canvas(rect.row_offset, rect.col_offset + rect.col_count - 1) = Cell{"┐"};
  canvas(rect.row_offset + rect.row_count - 1, rect.col_offset + rect.col_count - 1) = Cell{"┘"};

  // title
  if (name != "" && rect.col_count > name.size() + 3)
  {
    canvas(rect.row_offset, rect.col_offset + NAME_OFFSET) = Cell{" "};
    for (size_t i = 0; i < name.size(); ++i)
    {
      canvas(rect.row_offset, rect.col_offset + NAME_OFFSET + i + 1) = Cell{std::string(1, name[i])};
    }
    canvas(rect.row_offset, rect.col_offset + NAME_OFFSET + name.size() + 1) = Cell{" "};
  }
}
