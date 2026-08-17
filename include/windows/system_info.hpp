#pragma once

#include <string>

#include "canvas.hpp"
#include "drawing/draw_window_frame.hpp"
#include "raw_data/raw_data.hpp"
#include "windows/window.hpp"

class SystemInfoWindow : public Window
{
public:
  SystemInfoWindow(
      std::string name,
      Rect rect,
      const RawData::CpuThreads &cpu_threads,
      const RawData::CpuInfo &cpu_info,
      const RawData::KernelInfo &kernel_info)
      : Window(std::move(name), rect), cpu_threads_(cpu_threads), cpu_info_(cpu_info), kernel_info_(kernel_info)
  {
  }

  static size_t
  width(const RawData::CpuThreads &cpu_threads, const RawData::CpuInfo &cpu_info, const RawData::KernelInfo &kernel_info)
  {
    return std::max(heading_line(kernel_info).size(), cpu_line(cpu_threads, cpu_info).size()) + 2;
  }

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), name());

    const Rect system_info_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 2, rect().row_count - 2, rect().col_count - 2};
    canvas.copy_n(
        system_info_rect.row_offset,
        system_info_rect.col_offset,
        heading_line(kernel_info_),
        system_info_rect.col_count - 1);

    canvas.copy_n(
        system_info_rect.row_offset + 1,
        system_info_rect.col_offset,
        cpu_line(cpu_threads_, cpu_info_),
        system_info_rect.col_count - 1);
  }

private:
  const RawData::CpuThreads cpu_threads_;
  const RawData::CpuInfo cpu_info_;
  const RawData::KernelInfo kernel_info_;

  static std::string heading_line(const RawData::KernelInfo &kernel_info)
  {
    return std::format(
        "{}: {} {} ({})", kernel_info.nodename, kernel_info.sysname, kernel_info.release, kernel_info.machine);
  }

  static std::string cpu_line(const RawData::CpuThreads &cpu_threads, const RawData::CpuInfo &cpu_info)
  {
    return std::format("CPU: {} ({}C/{}T)", cpu_info.model_name, cpu_info.cpu_cores, cpu_threads.value);
  }
};
