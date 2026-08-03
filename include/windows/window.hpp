#pragma once

#include <string>

#include "canvas.hpp"

struct Rect
{
  size_t row_offset;
  size_t col_offset;
  size_t row_count;
  size_t col_count;
};

class Window
{
public:
  Window(std::string name, Rect rect) : name_(std::move(name)), rect_(rect) {}
  virtual ~Window() = default;

  virtual void draw(Canvas &canvas) const = 0;

  const std::string &name() const { return name_; }

  Rect rect() const { return rect_; }

private:
  const std::string name_;
  const Rect rect_;
};
