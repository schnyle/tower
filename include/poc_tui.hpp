#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <format>
#include <optional>
#include <thread>
#include <unistd.h>

#include "bar_plot_window.hpp"
#include "logger.hpp"
#include "meminfo.hpp"
#include "net_dev.hpp"
#include "ring_buffer.hpp"
#include "screen_buffer.hpp"
#include "stat.hpp"
#include "tui.hpp"

inline void poc_tui(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  const Tui tui;

  auto [terminal_rows, terminal_cols] = tui.get_size(); // move to an eventual tui_render() call
  ScreenBuffer screen_buffer(terminal_rows, terminal_cols);

  Stat last_stat;
  NetDev last_net_dev{-1, -1};
  double total_memory = 0;
  auto last_net_dev_read = std::chrono::steady_clock::now();

  RingBuffer<double> cpu_load_rb(50000);
  RingBuffer<double> mem_available_rb(50000);
  RingBuffer<double> download_rb(50000);
  RingBuffer<double> upload_rb(50000);

  std::deque<double> download_history;
  std::deque<double> upload_history;

  BarPlotWindow cpu_load_w{
      .row_offset = 0,
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 3),
      .col_count = terminal_cols,
      .title = "cpu load",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f}%", v * 100); },
      .data_rb = cpu_load_rb};

  BarPlotWindow available_mem_w{
      .row_offset = static_cast<size_t>(terminal_rows / 3),
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 3),
      .col_count = terminal_cols,
      .title = "available memory",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f} GB", v / 1024. / 1024.); },
      .data_rb = mem_available_rb};

  BarPlotWindow download_w{
      .row_offset = static_cast<size_t>(terminal_rows * 2 / 3),
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 3 / 2),
      .col_count = terminal_cols,
      .title = "download",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f} B/s", v); },
      .data_rb = download_rb};

  BarPlotWindow upload_w{
      .row_offset = static_cast<size_t>((terminal_rows * 2 / 3) + terminal_rows / 3 / 2),
      .col_offset = 0,
      .row_count = static_cast<size_t>(terminal_rows / 3 / 2),
      .col_count = terminal_cols,
      .title = "upload",
      .ymin = 0.,
      .ymax = 1.,
      .format_value = [](double v) { return std::format("{:.1f} B/s", v); },
      .data_rb = upload_rb};

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

    if (const auto net_dev = get_proc_net_dev())
    {
      auto now = std::chrono::steady_clock::now();
      double elapsed_seconds = std::chrono::duration<double>(now - last_net_dev_read).count();

      if (last_net_dev.rx_bytes != -1)
      {
        double download = (net_dev->rx_bytes - last_net_dev.rx_bytes) / elapsed_seconds;
        download_rb.push(download);
        download_history.push_back(download);
        if (download_history.size() > download_w.col_count - 2)
        {
          download_history.pop_front();
        }
        if (const auto max_it = std::max_element(download_history.begin(), download_history.end());
            max_it != download_history.end() && *max_it > 0)
        {
          download_w.ymax = *max_it;
        }
      }

      if (last_net_dev.tx_bytes)
      {
        double upload = (net_dev->tx_bytes - last_net_dev.tx_bytes) / elapsed_seconds;
        upload_rb.push(upload);
        upload_history.push_back(upload);
        if (upload_history.size() > upload_w.col_count - 2)
        {
          upload_history.pop_front();
        }
        if (const auto max_it = std::max_element(upload_history.begin(), upload_history.end());
            max_it != upload_history.end() && *max_it > 0)
        {
          upload_w.ymax = *max_it;
        }
      }

      last_net_dev = *net_dev;
      last_net_dev_read = now;
    }

    draw_bar_plot_window(screen_buffer.back_buf(), cpu_load_w);
    draw_bar_plot_window(screen_buffer.back_buf(), available_mem_w);
    draw_bar_plot_window(screen_buffer.back_buf(), download_w);
    draw_bar_plot_window(screen_buffer.back_buf(), upload_w);
    screen_buffer.draw();

    next += std::chrono::milliseconds(100);
    std::this_thread::sleep_until(next);
  }
}
