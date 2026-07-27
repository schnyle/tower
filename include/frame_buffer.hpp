#pragma once

#include <memory>
#include <string>

#include "canvas.hpp"

class FrameBuffer
{
public:
  FrameBuffer(unsigned short rows, unsigned short cols);

  Canvas &back_buf() { return *back_; }

  void draw();

private:
  const unsigned short rows_;
  const unsigned short cols_;

  std::string frame_;

  std::unique_ptr<Canvas> front_;
  std::unique_ptr<Canvas> back_;
};
