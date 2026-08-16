#pragma once

#include <format>
#include <functional>
#include <string>

#include "canvas.hpp"
#include "drawing/draw_bar_plot.hpp"
#include "logger.hpp"
#include "ring_buffer.hpp"
#include "windows/bar_plot/bar_plot.hpp"

class SingleBarPlotWindow : public BarPlotWindow
{
public:
  SingleBarPlotWindow(
      std::string name,
      Rect rect,
      double ymin,
      double ymax,
      bool dynamic_ymax,
      bool never_empty,
      Color color,
      std::function<std::string(double)> format_value,
      const RingBuffer<double> &data_rb)
      : BarPlotWindow(std::move(name), rect, ymin, ymax, dynamic_ymax, never_empty, format_value), color_(color),
        data_rb_(data_rb)
  {
  }

  void draw_bar_plot_impl(Canvas &canvas, const Rect bar_plot_rect) const override
  {
    double effective_ymax_ = effective_ymax(bar_plot_rect.col_count);
    draw_bar_plot(canvas, bar_plot_rect, ymin(), effective_ymax_, color_, data_rb_, false, never_empty());
    canvas.copy_n(bar_plot_rect.row_offset, bar_plot_rect.col_offset, format_value()(effective_ymax_));
  }

private:
  Color color_;
  const RingBuffer<double> &data_rb_;

  std::string heading() const override
  {
    std::string heading = name();
    if (const auto val = data_rb_.newest())
    {
      heading = std::format("{} {}", name(), format_value()(*val));
    }
    return heading;
  }

  double effective_ymax(const size_t col_count) const
  {
    if (!dynamic_ymax())
    {
      return ymax();
    }

    const auto max_displayed_value = find_max_displayed_value(data_rb_, col_count);
    if (!max_displayed_value)
    {
      return ymax();
    }

    double effective_ymax = *max_displayed_value;
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
