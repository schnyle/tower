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
#include "format_utils.hpp"
#include "frame_buffer.hpp"
#include "get_procs_data.hpp"
#include "get_system_info.hpp"
#include "logger.hpp"
#include "parsers/file.hpp" // for parse_next_token; TODO: decide where this helper should live if it has other users
#include "parsers/meminfo.hpp"
#include "parsers/net_dev.hpp"
#include "parsers/stat.hpp"
#include "ring_buffer.hpp"
#include "tui.hpp"
#include "windows/bar_plot.hpp"
#include "windows/process_list.hpp"
#include "windows/system_info.hpp"
#include "windows/user_input_window.hpp"

static constexpr int METRIC_POLL_MS = 1000;
static constexpr int INPUT_POLL_MS = 16;
static constexpr int DATA_POINTS = 50000;

enum class ProcListSortKey
{
  Cpu,
  Mem,
  Pid,
  Name
};

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
        user_input_window_{
            "user input",
            Rect{static_cast<size_t>(tui_size_.rows) - 1, 0, 1, tui_size_.cols},
            user_input_buf_},
        cpu_load_window_{
            "cpu load",
            rect_from_fractions(tui_size_, 0., 0., 1. / 6., 3. / 4.),
            0.,
            100.,
            Color::red(),
            [](double v) { return std::format("{:.1f}%", v); },
            cpu_load_pct_rb_},
        proc_cpu_load_window_{
            "proc cpu load",
            rect_from_fractions(tui_size_, 1. / 6., 0., 1. / 6., 3. / 4.),
            0.,
            100.,
            Color::red(),
            [](double v) { return format_percent(v); },
            proc_cpu_load_pct_rb_},
        available_memory_window_{
            "available memory",
            rect_from_fractions(tui_size_, 1. / 3., 0., 1. / 6., 3. / 4.),
            0.,
            1.,
            Color::green(),
            [](double v) { return format_memory_size(v); },
            mem_available_rb_},
        proc_memory_used_window_{
            "proc used memory",
            rect_from_fractions(tui_size_, (1. / 3.) + (1. / 6.), 0., 1. / 6., 3. / 4.),
            0.,
            1.,
            Color::green(),
            [](double v) { return format_memory_size(v); },
            proc_mem_used_rb_},
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
        system_info_window_{"tower", rect_from_fractions(tui_size_, 0., 3. / 4., 1. / 11., 1. / 4.), system_info_}
  {
    LOG_INFO("starting tower with terminal size: ", tui_.get_size().rows, " rows x ", tui_.get_size().cols, " col");
  }

  void run()
  {
    auto now = std::chrono::steady_clock::now();
    auto tick_start = now;
    auto next_input_tick = now;
    auto next_metric_tick = now;

    while (running_)
    {
      now = std::chrono::steady_clock::now();

      if (now >= next_input_tick)
      {
        tick_start = std::chrono::steady_clock::now();
        next_input_tick += std::chrono::milliseconds(INPUT_POLL_MS);
        handle_keyboard_input();
        if (!running_)
        {
          break;
        }
        user_input_window_.draw(frame_buffer_.back_buf());

        warn_if_overrun("input", tick_start, INPUT_POLL_MS);
      }

      if (now >= next_metric_tick)
      {
        tick_start = std::chrono::steady_clock::now();
        next_metric_tick += std::chrono::milliseconds(METRIC_POLL_MS);

        current_stat_ = collect<StatParser>("/proc/stat");
        current_mem_info_ = collect<MemInfoParser>("/proc/meminfo");
        current_net_dev_ = collect<NetDevParser>("/proc/net/dev");

        if (const auto pid = selected_pid_)
        {
          current_proc_stat_ = collect<ProcStatParser>(std::format("/proc/{}/stat", *pid));
          current_proc_status_ = collect<ProcStatusParser>(std::format("/proc/{}/status", *pid));
        }

        update_stat();
        update_proc_cpu_usage();
        update_meminfo();
        update_proc_mem_usage();
        update_net_dev();
        update_proc_data();

        cpu_load_window_.draw(frame_buffer_.back_buf());
        proc_cpu_load_window_.draw(frame_buffer_.back_buf());
        available_memory_window_.draw(frame_buffer_.back_buf());
        proc_memory_used_window_.draw(frame_buffer_.back_buf());
        receive_bytes_window_.draw(frame_buffer_.back_buf());
        transmit_bytes_window_.draw(frame_buffer_.back_buf());
        process_list_window_.draw(frame_buffer_.back_buf());
        system_info_window_.draw(frame_buffer_.back_buf());

        warn_if_overrun("metric", tick_start, METRIC_POLL_MS);
      }

      frame_buffer_.draw();
      std::this_thread::sleep_until(std::min(next_input_tick, next_metric_tick));
    }
  }

private:
  bool running_ = true;
  std::optional<int> selected_pid_;
  ProcListSortKey proc_list_sort_key_ = ProcListSortKey::Cpu;

  const Tui tui_;
  TerminalSize tui_size_;
  FrameBuffer frame_buffer_;

  std::string user_input_buf_ = "";

  const int clock_tick_ = sysconf(_SC_CLK_TCK);
  const SystemInfo system_info_ = get_system_info();

  double total_memory_ = 0;

  RingBuffer<double> cpu_load_pct_rb_{DATA_POINTS};
  RingBuffer<double> proc_cpu_load_pct_rb_{DATA_POINTS};
  RingBuffer<double> mem_available_rb_{DATA_POINTS};
  RingBuffer<double> proc_mem_used_rb_{DATA_POINTS};
  RingBuffer<double> download_rb_{DATA_POINTS};
  RingBuffer<double> upload_rb_{DATA_POINTS};

  std::vector<ProcData> proc_data_;

  UserInputWindow user_input_window_;
  BarPlotWindow cpu_load_window_;
  BarPlotWindow proc_cpu_load_window_;
  BarPlotWindow available_memory_window_;
  BarPlotWindow proc_memory_used_window_;
  BarPlotWindow receive_bytes_window_;
  BarPlotWindow transmit_bytes_window_;
  ProcessListWindow process_list_window_;
  SystemInfoWindow system_info_window_;

  std::optional<Poll<Stat>> current_stat_ = std::nullopt;
  std::optional<Poll<ProcStat>> current_proc_stat_ = std::nullopt;
  std::optional<Poll<MemInfo>> current_mem_info_ = std::nullopt;
  std::optional<Poll<ProcStatus>> current_proc_status_ = std::nullopt;
  std::optional<Poll<NetDev>> current_net_dev_ = std::nullopt;

  void warn_if_overrun(std::string_view tick_name, std::chrono::steady_clock::time_point start, int target_ms)
  {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::milliseconds(target_ms))
    {
      const auto actual_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
      LOG_WARNING(std::format("{} tick overran (target: {}ms, actual {}ms)", tick_name, target_ms, actual_ms));
    }
  }

  void handle_keyboard_input()
  {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1)
    {
      if (c == 0x03) // Ctrl + C; TODO: re-enable ISIG and handle real SIGINT (more complex termios manipulation)
      {
        running_ = false;
      }
      else if (c == '\n' || c == '\r') // enter
      {
        if (execute_command(user_input_buf_))
        {
          user_input_buf_ = "";
        }
      }
      else if (c == 0x7F || c == 0x08) // backspace
      {
        if (!user_input_buf_.empty())
        {
          user_input_buf_.pop_back();
        }
      }
      else if (('a' <= c && c <= 'z') || (c <= 'A' && c <= 'Z') || ('0' <= c && c <= '9') || (c == ' '))
      {
        user_input_buf_ += c;
      }
    }
  }

  bool execute_command(std::string_view input)
  {
    bool success = false;
    const std::string_view cmd = parse_next_token(input);

    if (cmd == "set")
    {
      const std::string_view subcmd = parse_next_token(input);
      if (subcmd == "pid")
      {
        const std::string_view arg = parse_next_token(input);
        int pid;
        const auto [ptr, ec] = std::from_chars(arg.data(), arg.data() + arg.size(), pid);
        if (ec == std::errc{} && ptr == arg.data() + arg.size())
        {
          selected_pid_ = pid;
          proc_cpu_load_pct_rb_.clear();
          proc_mem_used_rb_.clear();
          success = true;
        }
      }
      else if (subcmd == "procsort")
      {
        const std::string_view arg = parse_next_token(input);
        success = true;
        if (arg == "cpu")
        {
          proc_list_sort_key_ = ProcListSortKey::Cpu;
        }
        else if (arg == "mem")
        {
          proc_list_sort_key_ = ProcListSortKey::Mem;
        }
        else if (arg == "pid")
        {
          proc_list_sort_key_ = ProcListSortKey::Pid;
        }
        else if (arg == "name")
        {
          proc_list_sort_key_ = ProcListSortKey::Name;
        }
        else
        {
          user_input_buf_ = std::format("unrecognized arg to 'set procsort': {}", arg);
          success = false;
        }
      }
      else
      {
        user_input_buf_ = std::format("unrecognized subcommand to 'set': {}", subcmd);
      }
    }

    return success;
  }

  void repeat_last_rb_value(RingBuffer<double> &rb)
  {

    if (const auto newest = rb.newest())
    {
      rb.push(*newest);
    }
  }

  void update_stat()
  {
    static std::optional<Stat> previous_stat;

    if (!current_stat_)
    {
      repeat_last_rb_value(cpu_load_pct_rb_);
      return;
    }

    if (previous_stat)
    {
      CpuTimes delta = current_stat_->value.cpu_times - previous_stat->cpu_times;
      long busy_time = delta.user + delta.nice + delta.system + delta.irq + delta.softirq + delta.steal;
      long total_time = busy_time + delta.idle + delta.iowait;
      double usage = static_cast<double>(busy_time) / static_cast<double>(total_time) * 100;
      cpu_load_pct_rb_.push(usage);
    }

    previous_stat = current_stat_->value;
  }

  void update_proc_cpu_usage()
  {
    static std::optional<long long> previous_jiffies = std::nullopt;
    static std::optional<std::chrono::steady_clock::time_point> previous_read_time = std::nullopt;
    static int current_pid = -1;

    if (const auto pid = selected_pid_)
    {
      if (current_pid != *selected_pid_)
      {
        current_pid = *selected_pid_;
        previous_jiffies = std::nullopt;
        previous_read_time = std::nullopt;
      }
    }

    if (!current_proc_stat_)
    {
      repeat_last_rb_value(proc_cpu_load_pct_rb_);
      return;
    }

    const long long current_proc_jiffies = current_proc_stat_->value.stime + current_proc_stat_->value.utime;
    if (previous_jiffies && previous_read_time)
    {
      const double elapsed_seconds =
          std::chrono::duration<double>(current_proc_stat_->read_time - *previous_read_time).count();
      const double usage_pct = (current_proc_jiffies - *previous_jiffies) / (elapsed_seconds * clock_tick_) * 100 /
                               system_info_.cpu_threads;
      proc_cpu_load_pct_rb_.push(usage_pct);
    }

    previous_jiffies = current_proc_jiffies;
    previous_read_time = current_proc_stat_->read_time;
  }

  void update_meminfo()
  {
    if (!current_mem_info_)
    {
      repeat_last_rb_value(mem_available_rb_);
      return;
    }

    mem_available_rb_.push(static_cast<double>(current_mem_info_->value.mem_available_kb));
    total_memory_ = current_mem_info_->value.mem_total_kb;
    available_memory_window_.set_ymax(total_memory_);
    proc_memory_used_window_.set_ymax(total_memory_);
  }

  void update_proc_mem_usage()
  {
    if (!current_proc_status_)
    {
      repeat_last_rb_value(proc_mem_used_rb_);
      return;
    }

    proc_mem_used_rb_.push(static_cast<double>(current_proc_status_->value.vm_rss_kb));
  }

  double get_net_dev_ymax(double bytes_per_sec)
  {
    double result = 1.0;
    while (result < bytes_per_sec)
    {
      result *= 10;
    }
    return result;
  }

  void update_net_dev()
  {
    static NetDev previous_net_dev{-1, -1};
    static std::chrono::steady_clock::time_point previous_read_time = std::chrono::steady_clock::now();
    static std::deque<double> download_history_;
    static std::deque<double> upload_history_;

    if (!current_net_dev_)
    {
      repeat_last_rb_value(download_rb_);
      repeat_last_rb_value(upload_rb_);
      return;
    }

    double elapsed_seconds = std::chrono::duration<double>(current_net_dev_->read_time - previous_read_time).count();

    if (previous_net_dev.rx_bytes != -1)
    {
      double download = (current_net_dev_->value.rx_bytes - previous_net_dev.rx_bytes) / elapsed_seconds;
      download_rb_.push(download);
      download_history_.push_back(download);
      if (download_history_.size() > receive_bytes_window_.rect().col_count - 2)
      {
        download_history_.pop_front();
      }
      if (const auto max_it = std::max_element(download_history_.begin(), download_history_.end());
          max_it != download_history_.end() && *max_it > 0)
      {
        receive_bytes_window_.set_ymax(get_net_dev_ymax(*max_it));
      }
    }

    if (previous_net_dev.tx_bytes != -1)
    {
      double upload = (current_net_dev_->value.tx_bytes - previous_net_dev.tx_bytes) / elapsed_seconds;
      upload_rb_.push(upload);
      upload_history_.push_back(upload);
      if (upload_history_.size() > transmit_bytes_window_.rect().col_count - 2)
      {
        upload_history_.pop_front();
      }
      if (const auto max_it = std::max_element(upload_history_.begin(), upload_history_.end());
          max_it != upload_history_.end() && *max_it > 0)
      {
        transmit_bytes_window_.set_ymax(get_net_dev_ymax(*max_it));
      }
    }

    previous_net_dev = current_net_dev_->value;
    previous_read_time = current_net_dev_->read_time;
  }

  void update_proc_data()
  {
    static std::chrono::steady_clock::time_point previous_read_time = std::chrono::steady_clock::now();
    static std::unordered_map<int, long long> last_proc_jiffies_;

    const auto current_read_time = std::chrono::steady_clock::now();
    auto proc_data = get_procs_data();

    proc_data_.clear();
    proc_data_.reserve(proc_data.size());

    double elapsed_seconds = std::chrono::duration<double>(current_read_time - previous_read_time).count();
    std::unordered_set<int> current_pids;

    for (auto &pd : proc_data)
    {
      current_pids.insert(pd.pid);
      const long long current_jiffies = pd.utime + pd.stime;
      if (last_proc_jiffies_.contains(pd.pid))
      {
        const long long last_jiffies = last_proc_jiffies_[pd.pid];
        pd.cpu_usage_pct = (current_jiffies - last_jiffies) / (elapsed_seconds * clock_tick_) * 100 /
                           system_info_.cpu_threads;
      }
      proc_data_.push_back(pd);

      last_proc_jiffies_[pd.pid] = current_jiffies;
    }

    std::sort(
        proc_data_.begin(),
        proc_data_.end(),
        [this](const ProcData &a, const ProcData &b)
        {
          switch (proc_list_sort_key_)
          {
          case ProcListSortKey::Cpu:
            return a.cpu_usage_pct > b.cpu_usage_pct;
          case ProcListSortKey::Mem:
            return a.mem_usage_kb > b.mem_usage_kb;
          case ProcListSortKey::Pid:
            return a.pid < b.pid;
          case ProcListSortKey::Name:
            return a.name < b.name;
          }
          return false; // unreachable
        });

    std::erase_if(last_proc_jiffies_, [&](const auto &entry) { return !current_pids.contains(entry.first); });

    previous_read_time = current_read_time;
  }
};
