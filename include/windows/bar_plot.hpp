#pragma once

#include <format>
#include <functional>
#include <string>

#include "canvas.hpp"
#include "drawing/draw_bar_plot.hpp"
#include "drawing/draw_window_frame.hpp"
#include "ring_buffer.hpp"
#include "windows/window.hpp"

class BarPlotWindow : public Window
{
public:
  BarPlotWindow(
      std::string name,
      Rect rect,
      double ymin,
      double ymax,
      Color color,
      std::function<std::string(double)> format_value,
      RingBuffer<double> &data_rb)
      : Window(std::move(name), rect), ymin_(ymin), ymax_(ymax), color_(color), format_value_(std::move(format_value)),
        data_rb_(data_rb)
  {
  }

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), heading());

    const Rect bar_plot_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 1, rect().row_count - 2, rect().col_count - 2};
    draw_bar_plot(canvas, bar_plot_rect, ymin_, ymax_, color_, data_rb_);
  }

  void set_ymax(double ymax) { ymax_ = ymax; }

private:
  double ymin_, ymax_;
  Color color_;
  std::function<std::string(double)> format_value_;
  const RingBuffer<double> &data_rb_;

  std::string heading() const
  {
    std::string heading = name();
    if (const auto val = data_rb_.newest())
    {
      heading = std::format("{} {}", name(), format_value_(*val));
    }
    return heading;
  }
};
