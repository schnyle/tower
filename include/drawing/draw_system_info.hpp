#pragma once

#include <format>
#include <string>

#include "canvas.hpp"
#include "get_system_info.hpp"
#include "windows/window.hpp"

inline void draw_system_info(Canvas &canvas, const Rect rect, const SystemInfo system_info)
{
  const std::string heading = std::format(
      "{}: {} {} ({})", system_info.nodename, system_info.sysname, system_info.release, system_info.machine);
  canvas.copy_n(rect.row_offset, rect.col_offset, heading, rect.col_count - 1);

  const std::string cpu_info = std::format(
      "CPU: {} ({}C/{}T)", system_info.cpu_model, system_info.cpu_cores, system_info.cpu_threads);
  canvas.copy_n(rect.row_offset + 1, rect.col_offset, cpu_info, rect.col_count - 1);
}
