#include <format>
#include <stdexcept>
#include <string>
#include <vector>

#include "screen_buffer.hpp"

ScreenBuffer::ScreenBuffer(unsigned short rows, unsigned short cols) : rows_(rows), cols_(cols)
{
  front_buf_ = std::vector<std::vector<std::string>>(rows_, std::vector<std::string>(cols_, " "));
  back_buf_ = std::vector<std::vector<std::string>>(rows_, std::vector<std::string>(cols_, " "));

  const int estimated_frame_size = rows_ * (cols_ * 2 + 2);
  frame_.reserve(estimated_frame_size);
}

void ScreenBuffer::draw()
{
  static constexpr std::string CURSOR_POSITION_FMT = "\033[{};{}H";
  frame_.clear();

  int last_written_i = -1;
  int last_written_j = -1;
  for (unsigned short i = 0; i < rows_; ++i)
  {
    for (unsigned short j = 0; j < cols_; ++j)
    {
      if (back_buf_[i][j] == front_buf_[i][j])
      {
        continue;
      }

      if (i != last_written_i || j != last_written_j + 1)
      {
        frame_ += std::format(CURSOR_POSITION_FMT, i + 1, j + 1);
      }
      frame_ += back_buf_[i][j];
      last_written_i = i;
      last_written_j = j;
    }
  }

  fputs(frame_.c_str(), stdout);
  fflush(stdout);

  std::swap(front_buf_, back_buf_);
}
