#pragma once

#include <string>
#include <vector>

#include "build_bar_plot.hpp"
#include "build_window_frame.hpp"
#include "ring_buffer.hpp"

struct BarPlotWindow
{
  size_t row_offset, col_offset, row_count, col_count;
  std::string title;
  double ymin, ymax;
  const RingBuffer<double> &data_rb;
};

inline void draw_bar_plot_window(std::vector<std::vector<std::string>> &dest, const BarPlotWindow &w)
{
  build_window_frame(dest, w.row_offset, w.col_offset, w.row_count, w.col_count, w.title);
  build_bar_plot(dest, w.row_offset + 1, w.col_offset + 1, w.row_count - 2, w.col_count - 2, w.ymin, w.ymax, w.data_rb);
}
