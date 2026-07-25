#pragma once

#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <unistd.h>

#include "build_bar_plot.hpp"
#include "logger.hpp"
#include "meminfo.hpp"
#include "ring_buffer.hpp"
#include "screen_buffer.hpp"
#include "stat.hpp"
#include "tui.hpp"

constexpr int BAR_PLOT_ROWS = 25;

inline void poc_cpu_times_bar_plot(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  tui_enter();
  tui_clear();
  auto [terminal_rows, terminal_cols] = tui_get_size(); // move to an eventual tui_render() call

  ScreenBuffer screen_buffer(terminal_rows, terminal_cols);

  Stat last_stat;
  RingBuffer<double> cpu_load_rb(50000);
  RingBuffer<double> mem_available_rb(50000);

  double total_memory = 0;

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
    }

    auto [terminal_rows, terminal_cols] = tui_get_size(); // move to an eventual tui_render() call
    build_bar_plot(screen_buffer.back_buf(), 0, 0, BAR_PLOT_ROWS, terminal_cols, 0., 1., cpu_load_rb);
    build_bar_plot(
        screen_buffer.back_buf(), terminal_rows / 2, 0, BAR_PLOT_ROWS, terminal_cols, 0., total_memory, mem_available_rb);

    screen_buffer.draw();

    next += std::chrono::milliseconds(50);
    std::this_thread::sleep_until(next);
  }
}
