#pragma once

#include <chrono>
#include <cstdio>
#include <format>
#include <optional>
#include <thread>
#include <unistd.h>

#include "bar_plot_window.hpp"
#include "logger.hpp"
#include "meminfo.hpp"
#include "ring_buffer.hpp"
#include "screen_buffer.hpp"
#include "stat.hpp"
#include "tui.hpp"

inline void poc_cpu_times_bar_plot(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  const Tui tui;

  auto [terminal_rows, terminal_cols] = tui.get_size(); // move to an eventual tui_render() call
  ScreenBuffer screen_buffer(terminal_rows, terminal_cols);

  Stat last_stat;
  double total_memory = 0;
  RingBuffer<double> cpu_load_rb(50000);
  RingBuffer<double> mem_available_rb(50000);

  BarPlotWindow cpu_load_w{
      .row_offset = 0,
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 2),
      .col_count = terminal_cols,
      .title = "cpu load",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f}%", v * 100); },
      .data_rb = cpu_load_rb};

  BarPlotWindow available_mem_w{
      .row_offset = static_cast<size_t>(terminal_rows / 2),
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 2),
      .col_count = terminal_cols,
      .title = "available memory",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f} GB", v / 1024. / 1024.); },
      .data_rb = mem_available_rb};

  std::chrono::time_point next = std::chrono::steady_clock::now();
  while (true)
  {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
    {
      break;
    }

    if (const auto stat = get_proc_stat())
    {
      CpuTimes delta = stat->cpu_times - last_stat.cpu_times;
      long busy_time = delta.user + delta.nice + delta.system + delta.irq + delta.softirq + delta.steal;
      long total_time = busy_time + delta.idle + delta.iowait;
      double usage = static_cast<double>(busy_time) / static_cast<double>(total_time);
      cpu_load_rb.push(usage);
      last_stat = *stat;
    }

    if (const auto meminfo = get_proc_meminfo())
    {
      mem_available_rb.push(static_cast<double>(meminfo->mem_available_kb));
      total_memory = meminfo->mem_total_kb;
      available_mem_w.ymax = total_memory;
    }

    draw_bar_plot_window(screen_buffer.back_buf(), cpu_load_w);
    draw_bar_plot_window(screen_buffer.back_buf(), available_mem_w);
    screen_buffer.draw();

    next += std::chrono::milliseconds(100);
    std::this_thread::sleep_until(next);
  }
}
