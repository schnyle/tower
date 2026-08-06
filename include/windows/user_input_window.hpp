#pragma once

#include <string>

#include "canvas.hpp"
#include "windows/window.hpp"

class UserInputWindow : public Window
{
public:
  UserInputWindow(std::string name, Rect rect, const std::string &user_input)
      : Window(name, rect), user_input_(user_input)
  {
  }

  void draw(Canvas &canvas) const override
  {
    canvas.copy_n(rect().row_offset, rect().col_offset, std::string(rect().col_count, ' '), rect().col_count);
    canvas.copy_n(rect().row_offset, rect().col_offset, std::format("~ {}", user_input_), rect().col_count);
  }

private:
  const std::string &user_input_;
};
