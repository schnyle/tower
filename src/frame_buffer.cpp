#include <format>
#include <memory>
#include <string>

#include "frame_buffer.hpp"

FrameBuffer::FrameBuffer(unsigned short rows, unsigned short cols) : rows_(rows), cols_(cols)
{
  front_ = std::make_unique<Canvas>(rows, cols);
  back_ = std::make_unique<Canvas>(rows, cols);

  const int estimated_frame_size = rows_ * (cols_ * 2 + 2);
  frame_.reserve(estimated_frame_size);
}

void FrameBuffer::draw()
{
  static constexpr std::string CURSOR_POSITION_FMT = "\033[{};{}H";
  frame_.clear();

  int last_written_i = -1;
  int last_written_j = -1;
  for (unsigned short i = 0; i < rows_; ++i)
  {
    for (unsigned short j = 0; j < cols_; ++j)
    {
      if ((*back_)(i, j) == (*front_)(i, j))
      {
        continue;
      }

      if (i != last_written_i || j != last_written_j + 1)
      {
        frame_ += std::format(CURSOR_POSITION_FMT, i + 1, j + 1);
      }
      frame_ += (*back_)(i, j).bytes;
      last_written_i = i;
      last_written_j = j;
    }
  }

  fputs(frame_.c_str(), stdout);
  fflush(stdout);

  std::swap(front_, back_);
}
