#pragma once

#include <format>
#include <functional>
#include <string>

#include "canvas.hpp"
#include "drawing/draw_bar_plot.hpp"
#include "logger.hpp"
#include "ring_buffer.hpp"
#include "windows/bar_plot/bar_plot.hpp"

class DoubleBarPlotWindow : public BarPlotWindow
{
public:
  DoubleBarPlotWindow(
      std::string name,
      Rect rect,
      double ymin,
      double ymax,
      bool dynamic_ymax,
      bool never_empty,
      Color color_top,
      Color color_bottom,
      std::function<std::string(double)> format_value,
      const RingBuffer<double> &data_rb_top,
      const RingBuffer<double> &data_rb_bottom)
      : BarPlotWindow(std::move(name), rect, ymin, ymax, dynamic_ymax, never_empty, format_value),
        color_top_(color_top), color_bottom_(color_bottom), data_rb_top_(data_rb_top), data_rb_bottom_(data_rb_bottom)
  {
  }

  void draw_bar_plot_impl(Canvas &canvas, const Rect bar_plot_rect) const override
  {
    double effective_ymax_ = effective_ymax(bar_plot_rect.col_count);

    const Rect top_rect = Rect{
        bar_plot_rect.row_offset, bar_plot_rect.col_offset, bar_plot_rect.row_count / 2, bar_plot_rect.col_count};
    draw_bar_plot(canvas, top_rect, ymin(), effective_ymax_, color_top_, data_rb_top_, false, never_empty());

    const Rect bottom_rect = Rect{
        bar_plot_rect.row_offset + bar_plot_rect.row_count / 2,
        bar_plot_rect.col_offset,
        bar_plot_rect.row_count / 2,
        bar_plot_rect.col_count};
    draw_bar_plot(canvas, bottom_rect, ymin(), effective_ymax_, color_bottom_, data_rb_bottom_, true, never_empty());

    std::string formatted_ymax = format_value()(effective_ymax_);
    canvas.copy_n(bar_plot_rect.row_offset, bar_plot_rect.col_offset, formatted_ymax);
  }

private:
  Color color_top_;
  Color color_bottom_;
  const RingBuffer<double> &data_rb_top_;
  const RingBuffer<double> &data_rb_bottom_;

  std::string heading() const override
  {
    std::string heading = name();

    const auto top_val = data_rb_top_.newest();
    const auto bottom_val = data_rb_bottom_.newest();
    if (!top_val || !bottom_val)
    {
      return heading;
    }

    return std::format("{} - Download {} - Upload {}", name(), format_value()(*top_val), format_value()(*bottom_val));
  }

  double effective_ymax(const size_t col_count) const
  {
    if (!dynamic_ymax())
    {
      return ymax();
    }

    const auto max_displayed_value_top = find_max_displayed_value(data_rb_top_, col_count);
    const auto max_displayed_value_bottom = find_max_displayed_value(data_rb_bottom_, col_count);
    if (!max_displayed_value_top && !max_displayed_value_bottom)
    {
      return ymax();
    }

    double effective_ymax = ymax();
    if (max_displayed_value_top && !max_displayed_value_bottom)
    {
      effective_ymax = *max_displayed_value_top;
    }
    else if (!max_displayed_value_top && max_displayed_value_bottom)
    {
      effective_ymax = *max_displayed_value_bottom;
    }
    else
    {
      effective_ymax = std::max(*max_displayed_value_top, *max_displayed_value_bottom);
    }

    if (effective_ymax <= ymin())
    {
      if (effective_ymax < ymin())
      {
        LOG_WARNING(
            std::format("BarPlot Window {}: ymin set to {} but got data value {}", name(), ymin(), effective_ymax));
      }
      effective_ymax = ymax();
    }
    return effective_ymax;
  }
};
