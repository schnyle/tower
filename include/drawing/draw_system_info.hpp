#pragma once

#include <format>

#include "canvas.hpp"
#include "get_system_info.hpp"
#include "windows/window.hpp"

inline void draw_system_info(Canvas &canvas, const Rect rect, const SystemInfo system_info)
{
  canvas.copy_n(
      rect.row_offset,
      rect.col_offset,
      std::format(
          "{}: {} {} ({})", system_info.nodename, system_info.sysname, system_info.release, system_info.machine));

  canvas.copy_n(
      rect.row_offset + 1,
      rect.col_offset,
      std::format("CPU: {} ({} cores)", system_info.cpu_model, system_info.cpu_cores));
}
