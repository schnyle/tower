#pragma once

#include <string>
#include <vector>

class ScreenBuffer
{
public:
  ScreenBuffer(unsigned short rows, unsigned short cols);

  void blit(const std::vector<std::vector<std::string>> &buf, unsigned short row_offset, unsigned short col_offset);

  void draw();

private:
  const unsigned short rows_;
  const unsigned short cols_;
  std::vector<std::vector<std::string>> front_buf_;
  std::vector<std::vector<std::string>> back_buf_;
  std::string frame_;
};
