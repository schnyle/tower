#pragma once

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <format>
#include <optional>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>

#include "canvas.hpp"
#include "collect.hpp"
#include "frame_buffer.hpp"
#include "get_procs_data.hpp"
#include "get_system_info.hpp"
#include "logger.hpp"
#include "parsers/meminfo.hpp"
#include "parsers/net_dev.hpp"
#include "parsers/stat.hpp"
#include "ring_buffer.hpp"
#include "tui.hpp"
#include "windows/bar_plot.hpp"
#include "windows/process_list.hpp"
#include "windows/system_info.hpp"

static constexpr int INTERVAL_MS = 1000;

inline Rect rect_from_fractions(TerminalSize size, double row_frac, double col_frac, double row_span, double col_span)
{
  return Rect{
      static_cast<size_t>(size.rows * row_frac),
      static_cast<size_t>(size.cols * col_frac),
      static_cast<size_t>(size.rows * row_span),
      static_cast<size_t>(size.cols * col_span)};
}

class PocTui
{
public:
  PocTui()
      : tui_(), tui_size_(tui_.get_size()), frame_buffer_(tui_size_.rows, tui_size_.cols),
        cpu_load_window_{
            "cpu load",
            rect_from_fractions(tui_size_, 0., 0., 1. / 3., 3. / 4.),
            0.,
            1.,
            Color::red(),
            [](double v) { return std::format("{:.1f}%", v * 100); },
            cpu_load_rb_},
        available_memory_window_{
            "available memory",
            rect_from_fractions(tui_size_, 1. / 3., 0., 1. / 3., 3. / 4.),
            0.,
            1.,
            Color::green(),
            [](double v) { return std::format("{:.1f} GB", v / 1024. / 1024.); },
            mem_available_rb_},
        receive_bytes_window_{
            "download",
            rect_from_fractions(tui_size_, 2. / 3., 0., 1. / 6., 3. / 4.),
            0.,
            1.,
            Color::blue(),
            [](double v) { return std::format("{:.1f} B/s", v); },
            download_rb_},
        transmit_bytes_window_{
            "upload",
            rect_from_fractions(tui_size_, (2. / 3.) + (1. / 6.), 0., 1. / 6., 3. / 4.),
            0.,
            1.,
            Color::purple(),
            [](double v) { return std::format("{:.1f} B/s", v); },
            upload_rb_},
        process_list_window_{
            "processes",
            rect_from_fractions(tui_size_, 1. / 12., 3. / 4., 11. / 12., 1. / 4.),
            proc_data_,
        },
        system_info_window_{"tower", rect_from_fractions(tui_size_, 0., 3. / 4., 1. / 12., 1. / 4.), get_system_info()}
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

      cpu_load_window_.draw(frame_buffer_.back_buf());
      available_memory_window_.draw(frame_buffer_.back_buf());
      receive_bytes_window_.draw(frame_buffer_.back_buf());
      transmit_bytes_window_.draw(frame_buffer_.back_buf());
      process_list_window_.draw(frame_buffer_.back_buf());
      system_info_window_.draw(frame_buffer_.back_buf());
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

  const int clock_tick_ = sysconf(_SC_CLK_TCK);

  Stat last_stat_;
  NetDev last_net_dev_{-1, -1};
  double total_memory_ = 0;
  std::chrono::steady_clock::time_point last_net_dev_read_ = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point last_procs_read_ = std::chrono::steady_clock::now();
  std::unordered_map<int, long long> last_proc_jiffies_;

  RingBuffer<double> cpu_load_rb_{50000};
  RingBuffer<double> mem_available_rb_{50000};
  RingBuffer<double> download_rb_{50000};
  RingBuffer<double> upload_rb_{50000};

  std::deque<double> download_history_;
  std::deque<double> upload_history_;

  std::vector<ProcData> proc_data_;

  BarPlotWindow cpu_load_window_;
  BarPlotWindow available_memory_window_;
  BarPlotWindow receive_bytes_window_;
  BarPlotWindow transmit_bytes_window_;
  ProcessListWindow process_list_window_;
  SystemInfoWindow system_info_window_;

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
      available_memory_window_.set_ymax(total_memory_);
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
        if (download_history_.size() > receive_bytes_window_.rect().col_count - 2)
        {
          download_history_.pop_front();
        }
        if (const auto max_it = std::max_element(download_history_.begin(), download_history_.end());
            max_it != download_history_.end() && *max_it > 0)
        {
          receive_bytes_window_.set_ymax(*max_it);
        }
      }

      if (last_net_dev_.tx_bytes != -1)
      {
        double upload = (net_dev->tx_bytes - last_net_dev_.tx_bytes) / elapsed_seconds;
        upload_rb_.push(upload);
        upload_history_.push_back(upload);
        if (upload_history_.size() > transmit_bytes_window_.rect().col_count - 2)
        {
          upload_history_.pop_front();
        }
        if (const auto max_it = std::max_element(upload_history_.begin(), upload_history_.end());
            max_it != upload_history_.end() && *max_it > 0)
        {
          transmit_bytes_window_.set_ymax(*max_it);
        }
      }

      last_net_dev_ = *net_dev;
      last_net_dev_read_ = now;
    }
  }

  void update_proc_data()
  {
    auto now = std::chrono::steady_clock::now();
    auto proc_data = get_procs_data();

    proc_data_.clear();
    proc_data_.reserve(proc_data.size());

    double elapsed_seconds = std::chrono::duration<double>(now - last_procs_read_).count();
    std::unordered_set<int> current_pids;

    for (auto &pd : proc_data)
    {
      current_pids.insert(pd.pid);
      const long long current_jiffies = pd.utime + pd.stime;
      if (last_proc_jiffies_.contains(pd.pid))
      {
        const long long last_jiffies = last_proc_jiffies_[pd.pid];
        pd.cpu_usage_pct = (current_jiffies - last_jiffies) / (elapsed_seconds * clock_tick_) * 100 /
                           32; // TODO: replace with actual CPU count
      }
      proc_data_.push_back(pd);

      last_proc_jiffies_[pd.pid] = current_jiffies;
    }

    std::sort(
        proc_data_.begin(),
        proc_data_.end(),
        [](const ProcData &a, const ProcData &b) { return a.cpu_usage_pct > b.cpu_usage_pct; });

    std::erase_if(last_proc_jiffies_, [&](const auto &entry) { return !current_pids.contains(entry.first); });

    last_procs_read_ = now;
  }
};
