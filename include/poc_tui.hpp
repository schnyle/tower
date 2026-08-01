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
#include "canvas.hpp"
#include "collect.hpp"
#include "frame_buffer.hpp"
#include "get_procs_data.hpp"
#include "logger.hpp"
#include "parsers/meminfo.hpp"
#include "parsers/net_dev.hpp"
#include "parsers/stat.hpp"
#include "proc_list_window.hpp"
#include "ring_buffer.hpp"
#include "tui.hpp"

static constexpr int INTERVAL_MS = 100;

class PocTui
{
public:
  PocTui()
      : tui_(), tui_size_(tui_.get_size()), frame_buffer_(tui_size_.rows, tui_size_.cols),
        cpu_load_w_{
            .row_offset = 0,
            .col_offset = 0,
            .row_count = static_cast<size_t>(tui_size_.rows / 3),
            .col_count = static_cast<size_t>(tui_size_.cols * 3 / 4),
            .title = "cpu load",
            .ymin = 0.,
            .ymax = 1.,
            .color = Color::red(),
            .format_value = [](double v) { return std::format("{:.1f}%", v * 100); },
            .data_rb = cpu_load_rb_},
        available_mem_w_{
            .row_offset = static_cast<size_t>(tui_size_.rows / 3),
            .col_offset = 0,
            .row_count = static_cast<size_t>(tui_size_.rows / 3),
            .col_count = static_cast<size_t>(tui_size_.cols * 3 / 4),
            .title = "available memory",
            .ymin = 0.,
            .ymax = 1.,
            .color = Color::green(),
            .format_value = [](double v) { return std::format("{:.1f} GB", v / 1024. / 1024.); },
            .data_rb = mem_available_rb_},
        download_w_{
            .row_offset = static_cast<size_t>(tui_size_.rows * 2 / 3),
            .col_offset = 0,
            .row_count = static_cast<size_t>(tui_size_.rows / 3 / 2),
            .col_count = static_cast<size_t>(tui_size_.cols * 3 / 4),
            .title = "download",
            .ymin = 0.,
            .ymax = 1.,
            .color = Color::blue(),
            .format_value = [](double v) { return std::format("{:.1f} B/s", v); },
            .data_rb = download_rb_},
        upload_w_{
            .row_offset = static_cast<size_t>((tui_size_.rows * 2 / 3) + tui_size_.rows / 3 / 2),
            .col_offset = 0,
            .row_count = static_cast<size_t>(tui_size_.rows / 3 / 2),
            .col_count = static_cast<size_t>(tui_size_.cols * 3 / 4),
            .title = "upload",
            .ymin = 0.,
            .ymax = 1.,
            .color = Color::purple(),
            .format_value = [](double v) { return std::format("{:.1f} B/s", v); },
            .data_rb = upload_rb_},
        procs_w_{
            .row_offset = 0,
            .col_offset = static_cast<size_t>(tui_size_.cols * 3 / 4),
            .row_count = tui_size_.rows,
            .col_count = static_cast<size_t>(tui_size_.cols * 1 / 4),
            .title = "processes",
            .data = proc_data_,
        }
  {
    LOG_INFO("starting tower with terminal size: ", tui_.get_size().rows, " rows x ", tui_.get_size().cols, " col");
  }

  void run()
  {
    std::chrono::time_point start = std::chrono::steady_clock::now();
    std::chrono::time_point now = std::chrono::steady_clock::now();
    std::chrono::time_point next = std::chrono::steady_clock::now();

    while (true)
    {
      start = std::chrono::steady_clock::now();
      next = start + std::chrono::milliseconds(INTERVAL_MS); // TODO: handle long drift, e.g. process stalls for minutes

      char c;
      if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
      {
        break;
      }

      update_stat();
      update_meminfo();
      update_net_dev();
      update_proc_data();

      draw_bar_plot_window(frame_buffer_.back_buf(), cpu_load_w_);
      draw_bar_plot_window(frame_buffer_.back_buf(), available_mem_w_);
      draw_bar_plot_window(frame_buffer_.back_buf(), download_w_);
      draw_bar_plot_window(frame_buffer_.back_buf(), upload_w_);
      draw_proc_window(frame_buffer_.back_buf(), procs_w_);
      frame_buffer_.draw();

      if ((now = std::chrono::steady_clock::now()) > next)
      {
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        LOG_WARNING(std::format("loop target time: {}ms, actual time: {}", INTERVAL_MS, duration));
      }

      std::this_thread::sleep_until(next);
    }
  }

private:
  const Tui tui_;
  TerminalSize tui_size_;
  FrameBuffer frame_buffer_;

  Stat last_stat_;
  NetDev last_net_dev_{-1, -1};
  double total_memory_ = 0;
  std::chrono::steady_clock::time_point last_net_dev_read_ = std::chrono::steady_clock::now();

  RingBuffer<double> cpu_load_rb_{50000};
  RingBuffer<double> mem_available_rb_{50000};
  RingBuffer<double> download_rb_{50000};
  RingBuffer<double> upload_rb_{50000};

  std::deque<double> download_history_;
  std::deque<double> upload_history_;

  std::vector<ProcData> proc_data_;

  BarPlotWindow cpu_load_w_;
  BarPlotWindow available_mem_w_;
  BarPlotWindow download_w_;
  BarPlotWindow upload_w_;
  ProcListWindow procs_w_;

  void update_stat()
  {
    if (const auto stat = collect<StatParser>("/proc/stat"))
    {
      CpuTimes delta = stat->cpu_times - last_stat_.cpu_times;
      long busy_time = delta.user + delta.nice + delta.system + delta.irq + delta.softirq + delta.steal;
      long total_time = busy_time + delta.idle + delta.iowait;
      double usage = static_cast<double>(busy_time) / static_cast<double>(total_time);

      cpu_load_rb_.push(usage);

      last_stat_ = *stat;
    }
  }

  void update_meminfo()
  {
    if (const auto meminfo = collect<MemInfoParser>("/proc/meminfo"))
    {
      mem_available_rb_.push(static_cast<double>(meminfo->mem_available_kb));
      total_memory_ = meminfo->mem_total_kb;
      available_mem_w_.ymax = total_memory_;
    }
  }

  void update_net_dev()
  {
    if (const auto net_dev = collect<NetDevParser>("/proc/net/dev"))
    {
      auto now = std::chrono::steady_clock::now();
      double elapsed_seconds = std::chrono::duration<double>(now - last_net_dev_read_).count();

      if (last_net_dev_.rx_bytes != -1)
      {
        double download = (net_dev->rx_bytes - last_net_dev_.rx_bytes) / elapsed_seconds;
        download_rb_.push(download);
        download_history_.push_back(download);
        if (download_history_.size() > download_w_.col_count - 2)
        {
          download_history_.pop_front();
        }
        if (const auto max_it = std::max_element(download_history_.begin(), download_history_.end());
            max_it != download_history_.end() && *max_it > 0)
        {
          download_w_.ymax = *max_it;
        }
      }

      if (last_net_dev_.tx_bytes != -1)
      {
        double upload = (net_dev->tx_bytes - last_net_dev_.tx_bytes) / elapsed_seconds;
        upload_rb_.push(upload);
        upload_history_.push_back(upload);
        if (upload_history_.size() > upload_w_.col_count - 2)
        {
          upload_history_.pop_front();
        }
        if (const auto max_it = std::max_element(upload_history_.begin(), upload_history_.end());
            max_it != upload_history_.end() && *max_it > 0)
        {
          upload_w_.ymax = *max_it;
        }
      }

      last_net_dev_ = *net_dev;
      last_net_dev_read_ = now;
    }
  }

  void update_proc_data()
  {
    auto proc_data = get_procs_data();
    proc_data_.clear();
    proc_data_.reserve(proc_data.size());
    for (const auto &d : proc_data)
    {
      proc_data_.push_back(d);
    }
  }
};
