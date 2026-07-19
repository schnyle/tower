#pragma once

#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <unistd.h>

#include "build_bar_plot.hpp"
#include "logger.hpp"
#include "ring_buffer.hpp"
#include "stat.hpp"
#include "tui.hpp"

inline void poc_cpu_times_bar_plot(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  tui_enter();
  tui_clear();

  Stat last_stat;
  RingBuffer<double> cpu_load_rb(1000);

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

    auto [terminal_rows, terminal_cols] = tui_get_size(); // move to an eventual tui_render() call
    const auto bar_plot = build_bar_plot(terminal_rows, terminal_cols, 0., 1., cpu_load_rb);
    for (const auto &line : bar_plot)
    {
      for (const auto &cell : line)
      {
        printf("%s", cell.c_str());
      }
      printf("\n\r");
    }

    next += std::chrono::milliseconds(100);
    std::this_thread::sleep_until(next);

    tui_clear();
  }
}
