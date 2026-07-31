#pragma once

#include <format>
#include <functional>
#include <string>

#include "build_bar_plot.hpp"
#include "build_window_frame.hpp"
#include "canvas.hpp"
#include "ring_buffer.hpp"

struct BarPlotWindow
{
  size_t row_offset, col_offset, row_count, col_count;
  std::string title;
  double ymin, ymax;
  Color color;
  std::function<std::string(double)> format_value;
  const RingBuffer<double> &data_rb;
};

inline void draw_bar_plot_window(Canvas &canvas, const BarPlotWindow &w)
{
  std::string heading = w.title;
  if (const auto v = w.data_rb.newest())
  {
    heading = std::format("{} {}", w.title, w.format_value(*v));
  }

  build_window_frame(canvas, w.row_offset, w.col_offset, w.row_count, w.col_count, heading);
  build_bar_plot(
      canvas, w.row_offset + 1, w.col_offset + 1, w.row_count - 2, w.col_count - 2, w.ymin, w.ymax, w.color, w.data_rb);
}
