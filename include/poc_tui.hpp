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
#include "parsers/vmstat.hpp"
#include "ring_buffer.hpp"
#include "tui.hpp"
#include "windows/bar_plot.hpp"
#include "windows/process_list.hpp"
#include "windows/system_info.hpp"
#include "windows/user_input_window.hpp"

static constexpr int METRIC_POLL_MS = 1000;
static constexpr int INPUT_POLL_MS = 16;
static constexpr int DATA_POINTS = 50000;

// clang-format off
const std::vector<std::vector<std::string>> LAYOUT = {
    {"cpu",         "cpu",         "cpu",         "sys_info"},
    {"proc_cpu",    "proc_cpu",    "proc_cpu",    "proc_list"},
    {"mem",         "mem",         "mem",         "proc_list"},
    {"proc_mem",    "proc_mem",    "proc_mem",    "proc_list"},
    {"majflt",      "majflt",      "majflt",      "proc_list"},
    {"proc_majflt", "proc_majflt", "proc_majflt", "proc_list"},
    {"download",    "download",    "download",    "proc_list"},
    {"upload",      "upload",      "upload",      "proc_list"},
};
// clang-format on

enum class ProcListSortKey
{
  Cpu,
  Mem,
  Pid,
  Name
};

template <typename T> class DeltaTracker
{
public:
  std::optional<T> delta(T current_value)
  {
    std::optional<T> delta;
    if (previous_)
    {
      delta = current_value - *previous_;
    }
    previous_ = current_value;
    return delta;
  }

  void reset() { previous_ = std::nullopt; }

private:
  std::optional<T> previous_;
};

class ElapsedTimeTracker
{
public:
  std::optional<std::chrono::duration<double>> elapsed(std::chrono::steady_clock::time_point now)
  {
    std::optional<std::chrono::duration<double>> elapsed;
    if (previous_)
    {
      elapsed = std::chrono::duration<double>(now - *previous_);
    }
    previous_ = now;
    return elapsed;
  }

  void reset() { previous_ = std::nullopt; }

private:
  std::optional<std::chrono::steady_clock::time_point> previous_;
};

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
            layout_.at("cpu"),
            0.,
            100.,
            Color::red(),
            [](double v) { return std::format("{:.1f}%", v); },
            cpu_load_pct_rb_},
        proc_cpu_load_window_{
            "proc cpu load",
            layout_.at("proc_cpu"),
            0.,
            100.,
            Color::red(),
            [](double v) { return format_percent(v); },
            proc_cpu_load_pct_rb_},
        available_memory_window_{
            "available memory",
            layout_.at("mem"),
            0.,
            1.,
            Color::green(),
            [](double v) { return format_memory_size(v); },
            mem_available_rb_},
        proc_memory_used_window_{
            "proc used memory",
            layout_.at("proc_mem"),
            0.,
            1.,
            Color::green(),
            [](double v) { return format_memory_size(v); },
            proc_mem_used_rb_},
        major_page_fault_window_{
            "major page faults",
            layout_.at("majflt"),
            0.,
            1.,
            Color::green(),
            [](double v) { return std::format("{:.1f}/s", v); },
            major_page_fault_per_sec_rb_},
        proc_major_page_fault_window_{
            "proc major page faults",
            layout_.at("proc_majflt"),
            0.,
            1.,
            Color::green(),
            [](double v) { return std::format("{:.1f}/s", v); },
            proc_major_page_fault_per_sec_rb_},
        receive_bytes_window_{
            "download",
            layout_.at("download"),
            0.,
            1.,
            Color::blue(),
            [](double v) { return std::format("{:.1f} B/s", v); },
            download_rb_},
        transmit_bytes_window_{
            "upload",
            layout_.at("upload"),
            0.,
            1.,
            Color::purple(),
            [](double v) { return std::format("{:.1f} B/s", v); },
            upload_rb_},
        process_list_window_{
            "processes",
            layout_.at("proc_list"),
            proc_data_,
        },
        system_info_window_{"tower", layout_.at("sys_info"), system_info_}
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
        current_vm_stat_ = collect<VmStatParser>("/proc/vmstat");

        if (const auto pid = selected_pid_)
        {
          current_proc_stat_ = collect<ProcStatParser>(std::format("/proc/{}/stat", *pid));
          current_proc_status_ = collect<ProcStatusParser>(std::format("/proc/{}/status", *pid));
        }

        update_cpu_load_pct();
        update_proc_cpu_load();
        update_meminfo();
        update_proc_mem_usage();
        update_major_page_faults();
        update_proc_major_page_faults();
        update_net_dev();
        update_proc_data();

        cpu_load_window_.draw(frame_buffer_.back_buf());
        proc_cpu_load_window_.draw(frame_buffer_.back_buf());
        available_memory_window_.draw(frame_buffer_.back_buf());
        proc_memory_used_window_.draw(frame_buffer_.back_buf());
        major_page_fault_window_.draw(frame_buffer_.back_buf());
        proc_major_page_fault_window_.draw(frame_buffer_.back_buf());
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
  const std::unordered_map<std::string, Rect> layout_ = layout_to_rects(tui_size_, LAYOUT);
  FrameBuffer frame_buffer_;

  std::string user_input_buf_ = "";

  const int clock_tick_ = sysconf(_SC_CLK_TCK);
  const SystemInfo system_info_ = get_system_info();

  double total_memory_ = 0;

  RingBuffer<double> cpu_load_pct_rb_{DATA_POINTS};
  DeltaTracker<CpuTimes> cpu_times_delta_tracker_;
  RingBuffer<double> proc_cpu_load_pct_rb_{DATA_POINTS};
  DeltaTracker<long long> proc_cpu_jiffies_delta_tracker_;
  RingBuffer<double> mem_available_rb_{DATA_POINTS};
  RingBuffer<double> proc_mem_used_rb_{DATA_POINTS};
  RingBuffer<double> major_page_fault_per_sec_rb_{DATA_POINTS};
  DeltaTracker<double> maj_page_fault_delta_tracker_;
  RingBuffer<double> proc_major_page_fault_per_sec_rb_{DATA_POINTS};
  DeltaTracker<long unsigned> proc_maj_page_fault_delta_tracker_;
  RingBuffer<double> download_rb_{DATA_POINTS};
  DeltaTracker<double> rx_bytes_delta_tracker_;
  RingBuffer<double> upload_rb_{DATA_POINTS};
  DeltaTracker<double> tx_bytes_delta_tracker_;

  std::vector<ProcData> proc_data_;

  UserInputWindow user_input_window_;
  BarPlotWindow cpu_load_window_;
  BarPlotWindow proc_cpu_load_window_;
  BarPlotWindow available_memory_window_;
  BarPlotWindow proc_memory_used_window_;
  BarPlotWindow major_page_fault_window_;
  BarPlotWindow proc_major_page_fault_window_;
  BarPlotWindow receive_bytes_window_;
  BarPlotWindow transmit_bytes_window_;
  ProcessListWindow process_list_window_;
  SystemInfoWindow system_info_window_;

  std::optional<Poll<Stat>> current_stat_ = std::nullopt;
  ElapsedTimeTracker stat_elapsed_time_tracker_;
  std::optional<Poll<ProcStat>> current_proc_stat_ = std::nullopt;
  ElapsedTimeTracker proc_stat_elapsed_time_tracker_;
  std::optional<Poll<MemInfo>> current_mem_info_ = std::nullopt;
  ElapsedTimeTracker mem_info_elapsed_time_tracker_;
  std::optional<Poll<ProcStatus>> current_proc_status_ = std::nullopt;
  ElapsedTimeTracker proc_status_elapsed_time_tracker_;
  std::optional<Poll<NetDev>> current_net_dev_ = std::nullopt;
  ElapsedTimeTracker net_dev_elapsed_time_tracker_;
  std::optional<Poll<VmStat>> current_vm_stat_ = std::nullopt;
  ElapsedTimeTracker vm_stat_elapsed_time_tracker_;

  void warn_if_overrun(std::string_view tick_name, std::chrono::steady_clock::time_point start, int target_ms)
  {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::milliseconds(target_ms))
    {
      const auto actual_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
      LOG_WARNING(std::format("{} tick overran (target: {}ms, actual {}ms)", tick_name, target_ms, actual_ms));
    }
  }

  static std::unordered_map<std::string, Rect>
  layout_to_rects(TerminalSize terminal_size, const std::vector<std::vector<std::string>> &layout)
  {
    // TODO: add validation (non-zero, all inner vectors have same size)
    const size_t layout_rows = layout.size();
    const size_t layout_cols = layout[0].size();

    // reserve final row for user input
    --terminal_size.rows;

    // for now, ignore "extra" rows/cols that dont divide nicely into the layout dimensions
    terminal_size.rows = terminal_size.rows - (terminal_size.rows % layout_rows);
    terminal_size.cols = terminal_size.cols - (terminal_size.cols % layout_cols);

    std::unordered_map<std::string, Rect> result;

    size_t i;
    size_t window_row_count = 0;
    size_t window_col_count = 0;
    for (size_t row = 0; row < layout_rows; ++row)
    {
      for (size_t col = 0; col < layout_cols; ++col)
      {
        const std::string &ele = layout[row][col];
        if (result.contains(ele))
        {
          continue;
        }

        window_row_count = 1;
        window_col_count = 1;

        i = row;
        while (++i < layout_rows && layout[i][col] == ele)
        {
          ++window_row_count;
        }

        i = col;
        while (++i < layout_cols && layout[row][i] == ele)
        {
          ++window_col_count;
        }

        result[ele] = Rect{
            .row_offset = terminal_size.rows / layout_rows * row,
            .col_offset = terminal_size.cols / layout_cols * col,
            .row_count = terminal_size.rows / layout_rows * window_row_count,
            .col_count = terminal_size.cols / layout_cols * window_col_count};
      }
    }

    return result;
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
          proc_major_page_fault_per_sec_rb_.clear();

          proc_stat_elapsed_time_tracker_.reset();
          proc_status_elapsed_time_tracker_.reset();
          proc_cpu_jiffies_delta_tracker_.reset();
          proc_maj_page_fault_delta_tracker_.reset();

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

  void update_cpu_load_pct()
  {
    if (!current_stat_)
    {
      repeat_last_rb_value(cpu_load_pct_rb_);
      return;
    }

    if (const auto delta = cpu_times_delta_tracker_.delta(current_stat_->value.cpu_times))
    {
      long busy_time = delta->user + delta->nice + delta->system + delta->irq + delta->softirq + delta->steal;
      long idle_time = delta->idle + delta->iowait;
      double usage = static_cast<double>(busy_time) / static_cast<double>(busy_time + idle_time) * 100;
      cpu_load_pct_rb_.push(usage);
    }
  }

  void update_proc_cpu_load()
  {
    if (!current_proc_stat_)
    {
      repeat_last_rb_value(proc_cpu_load_pct_rb_);
      return;
    }

    const long long current_proc_jiffies = current_proc_stat_->value.stime + current_proc_stat_->value.utime;

    const auto delta = proc_cpu_jiffies_delta_tracker_.delta(current_proc_jiffies);
    const auto elapsed = proc_stat_elapsed_time_tracker_.elapsed(current_proc_stat_->read_time);
    if (delta && elapsed)
    {
      const double usage_pct = *delta / (elapsed->count() * clock_tick_) / system_info_.cpu_threads * 100;
      proc_cpu_load_pct_rb_.push(usage_pct);
    }
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

  void update_major_page_faults()
  {
    if (!current_vm_stat_)
    {
      repeat_last_rb_value(major_page_fault_per_sec_rb_);
      return;
    }

    const auto delta = maj_page_fault_delta_tracker_.delta(current_vm_stat_->value.pgmajfault);
    const auto elapsed = vm_stat_elapsed_time_tracker_.elapsed(current_vm_stat_->read_time);

    if (delta && elapsed)
    {
      const double page_fault_rate = *delta / elapsed->count();
      major_page_fault_per_sec_rb_.push(page_fault_rate);
      major_page_fault_window_.set_ymax(std::max(major_page_fault_window_.ymax(), page_fault_rate));
    }
  }

  void update_proc_major_page_faults()
  {
    if (!current_proc_stat_)
    {
      repeat_last_rb_value(proc_major_page_fault_per_sec_rb_);
      return;
    }

    const auto delta = proc_maj_page_fault_delta_tracker_.delta(current_proc_stat_->value.majflt);
    const auto elapsed = proc_stat_elapsed_time_tracker_.elapsed(current_proc_stat_->read_time);

    if (delta && elapsed)
    {
      const double page_fault_rate = *delta / elapsed->count();
      proc_major_page_fault_per_sec_rb_.push(page_fault_rate);
      proc_major_page_fault_window_.set_ymax(std::max(proc_major_page_fault_window_.ymax(), page_fault_rate));
    }
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
    static std::deque<double> download_history_;
    static std::deque<double> upload_history_;

    if (!current_net_dev_)
    {
      repeat_last_rb_value(download_rb_);
      repeat_last_rb_value(upload_rb_);
      return;
    }

    const auto rx_delta = rx_bytes_delta_tracker_.delta(current_net_dev_->value.rx_bytes);
    const auto tx_delta = tx_bytes_delta_tracker_.delta(current_net_dev_->value.tx_bytes);
    const auto elapsed = net_dev_elapsed_time_tracker_.elapsed(current_net_dev_->read_time);

    if (rx_delta && elapsed)
    {
      const double download_rate = *rx_delta / elapsed->count();
      download_rb_.push(download_rate);
      download_history_.push_back(download_rate);
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

    if (tx_delta && elapsed)
    {
      const double upload = *tx_delta / elapsed->count();
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
