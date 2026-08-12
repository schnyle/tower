#pragma once

#include <format>
#include <string>

#include "canvas.hpp"
#include "raw_data/raw_data.hpp"
#include "windows/window.hpp"

inline void draw_system_info(
    Canvas &canvas,
    const Rect rect,
    const RawData::CpuThreads &cpu_threads,
    const RawData::CpuInfo &cpu_info,
    const RawData::KernelInfo &kernel_info)
{
  const std::string heading_line = std::format(
      "{}: {} {} ({})", kernel_info.nodename, kernel_info.sysname, kernel_info.release, kernel_info.machine);
  canvas.copy_n(rect.row_offset, rect.col_offset, heading_line, rect.col_count - 1);

  const std::string cpu_line = std::format(
      "CPU: {} ({}C/{}T)", cpu_info.model_name, cpu_info.cpu_cores, cpu_threads.value);
  canvas.copy_n(rect.row_offset + 1, rect.col_offset, cpu_line, rect.col_count - 1);
}
