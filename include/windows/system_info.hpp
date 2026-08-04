#pragma once

#include <string>

#include "canvas.hpp"
#include "drawing/draw_system_info.hpp"
#include "drawing/draw_window_frame.hpp"
#include "get_system_info.hpp"
#include "windows/window.hpp"

class SystemInfoWindow : public Window
{
public:
  SystemInfoWindow(std::string name, Rect rect, SystemInfo system_info)
      : Window(name, rect), system_info_(std::move(system_info))
  {
  }

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), name());

    const Rect system_info_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 2, rect().row_count - 2, rect().col_count - 2};
    draw_system_info(canvas, system_info_rect, system_info_);
  }

private:
  const SystemInfo system_info_;
};
