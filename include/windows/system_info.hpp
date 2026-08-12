#pragma once

#include <string>

#include "canvas.hpp"
#include "drawing/draw_system_info.hpp"
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

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), name());

    const Rect system_info_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 2, rect().row_count - 2, rect().col_count - 2};
    draw_system_info(canvas, system_info_rect, cpu_threads_, cpu_info_, kernel_info_);
  }

private:
  const RawData::CpuThreads cpu_threads_;
  const RawData::CpuInfo cpu_info_;
  const RawData::KernelInfo kernel_info_;
};
