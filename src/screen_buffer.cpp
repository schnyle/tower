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

void ScreenBuffer::blit(
    const std::vector<std::vector<std::string>> &source_buf,
    unsigned short row_offset,
    unsigned short col_offset)
{
  const size_t source_rows = source_buf.size();
  const size_t source_cols = source_buf.at(0).size();

  if (col_offset + source_cols > cols_ || row_offset + source_rows > rows_)
  {
    const std::string msg = std::format(
        "ScreenBuffer::blit: source ({} rows x {} cols) at offset ({}, {}) exceeds buffer bounds ({} x {})",
        source_rows,
        source_cols,
        row_offset,
        col_offset,
        rows_,
        cols_);
    throw std::out_of_range(msg);
  }

  for (size_t i = 0; i < source_rows; ++i)
  {
    for (size_t j = 0; j < source_cols; ++j)
    {
      back_buf_[i + row_offset][j + col_offset] = source_buf[i][j];
    }
  }
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
