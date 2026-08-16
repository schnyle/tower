#pragma once

#include <functional>
#include <string>

#include "canvas.hpp"
#include "drawing/draw_window_frame.hpp"
#include "windows/window.hpp"

class BarPlotWindow : public Window
{
public:
  BarPlotWindow(
      std::string name,
      Rect rect,
      double ymin,
      double ymax,
      bool dynamic_ymax,
      bool never_empty,
      std::function<std::string(double)> format_value)
      : Window(std::move(name), rect), ymin_(ymin), ymax_(ymax), dynamic_ymax_(dynamic_ymax), never_empty_(never_empty),
        format_value_(format_value)
  {
  }

  double ymin() const { return ymin_; }

  double ymax() const { return ymax_; }

  bool dynamic_ymax() const { return dynamic_ymax_; }

  bool never_empty() const { return never_empty_; }

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), heading());

    const Rect bar_plot_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 1, rect().row_count - 2, rect().col_count - 2};

    draw_bar_plot_impl(canvas, bar_plot_rect);
  }

protected:
  const std::function<std::string(double)> &format_value() const { return format_value_; }

  virtual std::string heading() const = 0;

  virtual void draw_bar_plot_impl(Canvas &canvas, const Rect) const = 0;

private:
  double ymin_;
  double ymax_;
  const bool dynamic_ymax_;
  const bool never_empty_;
  std::function<std::string(double)> format_value_;
};
