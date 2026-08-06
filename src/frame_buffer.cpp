#include <format>
#include <string>

#include "frame_buffer.hpp"

FrameBuffer::FrameBuffer(unsigned short rows, unsigned short cols)
    : rows_(rows), cols_(cols), front_{rows, cols}, back_{rows, cols}
{
  const int estimated_frame_size = rows_ * (cols_ * 2 + 2);
  frame_.reserve(estimated_frame_size);
}

void FrameBuffer::draw()
{
  static constexpr std::string_view CURSOR_POSITION_FMT = "\033[{};{}H";
  static constexpr std::string_view CURSOR_COLOR_FMT = "\033[38;2;{};{};{}m";
  frame_.clear();

  int last_written_row = -1;
  int last_written_col = -1;
  for (unsigned short row = 0; row < rows_; ++row)
  {
    for (unsigned short col = 0; col < cols_; ++col)
    {
      if (back_(row, col) == front_(row, col))
      {
        continue;
      }

      front_(row, col) = back_(row, col);

      if (row != last_written_row || col != last_written_col + 1)
      {
        frame_ += std::format(CURSOR_POSITION_FMT, row + 1, col + 1);
      }

      const Color color = back_(row, col).color;
      if (color != current_color)
      {
        frame_ += std::format(CURSOR_COLOR_FMT, color.r, color.g, color.b);
        current_color = color;
      }

      frame_ += back_(row, col).bytes;
      last_written_row = row;
      last_written_col = col;
    }
  }

  fputs(frame_.c_str(), stdout);
  fflush(stdout);
}
