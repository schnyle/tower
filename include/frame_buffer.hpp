#pragma once

#include <string>

#include "canvas.hpp"

class FrameBuffer
{
public:
  FrameBuffer(unsigned short rows, unsigned short cols);

  Canvas &back_buf() { return back_; }

  void draw();

private:
  const unsigned short rows_;
  const unsigned short cols_;

  Canvas front_;
  Canvas back_;

  std::string frame_;
  Color current_color = Color::white();
};
