#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#include "format_utils.hpp"
#include "frame_buffer.hpp"
#include "logger.hpp"
#include "metric.hpp"
#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"
#include "raw_data/raw_data_cache.hpp"
#include "single_proc_info.hpp"
#include "tui.hpp"
#include "windows/bar_plot.hpp"
#include "windows/process_list.hpp"
#include "windows/system_info.hpp"
#include "windows/user_input_window.hpp"
#include "windows/window.hpp"

// config
static constexpr int USER_INPUT_INTERVAL_MS = 16;
static constexpr int COLLECTION_INTERVAL_MS = 100;
static constexpr int DATA_POINTS = 50000;

static constexpr double BAR_PLOTS_WIDTH_RATIO = 0.75;

struct MetricDefinition
{
  std::string name;
  Color color;
  double initial_ymin;
  std::function<double(const RawDataCache &)> initial_ymax;
  bool dynamic_ymax;
  std::function<std::string(double)> format;
  std::function<std::unique_ptr<Metric>(const RawDataCache &)> make;
};

static const std::vector<MetricDefinition> METRIC_DEFINITIONS = {
    {
        .name = "CPU Load",
        .color = Color::red(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = false,
        .format = [](const double v) { return std::format("{:.1f}%", v); },
        .make = make_cpu_load_metric,
    },
    {
        .name = "Available Memory",
        .color = Color::green(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &cache) { return cache.get<RawData::MemInfo>()->mem_total_kb; },
        .dynamic_ymax = false,
        .format = [](const double v) { return format_bytes_iec(v / 1024); },
        .make = make_gauge_metric<RawData::MemInfo, &RawData::MemInfo::mem_available_kb>,
    },
    {
        .name = "Major Page Faults",
        .color = Color(100, 50, 200),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = true,
        .format = [](const double v) { return std::format("{:.1f} per sec", v); },
        .make = make_delta_rate_metric<RawData::VmStat, &RawData::VmStat::pgmajfault>,
    },
    {
        .name = "Download",
        .color = Color::purple(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 10000.; },
        .dynamic_ymax = true,
        .format = [](const double v) { return std::format("{}/s", format_bytes_si(v)); },
        .make = make_delta_rate_metric<RawData::NetDev, &RawData::NetDev::rx_bytes>,
    },
    {
        .name = "Upload",
        .color = Color::blue(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 10000.; },
        .dynamic_ymax = true,
        .format = [](const double v) { return std::format("{}/s", format_bytes_si(v)); },
        .make = make_delta_rate_metric<RawData::NetDev, &RawData::NetDev::tx_bytes>,
    },
    {
        .name = "Process Major Page Faults",
        .color = Color(14, 27, 19),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = true,
        .format = [](const double v) { return std::format("{:.1f} per sec", v); },
        .make = make_delta_rate_metric<RawData::ProcStat, &RawData::ProcStat::majflt>,
    },
    {
        .name = "Process Memory Usage",
        .color = Color(47, 200, 150),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &cache) { return cache.get<RawData::MemInfo>()->mem_total_kb; },
        .dynamic_ymax = false,
        .format = [](const double v) { return format_bytes_iec(v / 1024); },
        .make = make_gauge_metric<RawData::ProcStatus, &RawData::ProcStatus::vm_rss_kb>,
    },
    {
        .name = "Process CPU Usage",
        .color = Color(150, 47, 120),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = false,
        .format = [](const double v) { return std::format("{:.1f}%", v); },
        .make = make_proc_cpu_usage_metric,
    },
};

class PocTui
{
public:
  PocTui()
  {
    LOG_INFO("starting tower with terminal size: ", tui_.get_size().rows, " rows x ", tui_.get_size().cols, " col");

    for (const auto &def : METRIC_DEFINITIONS)
    {
      metrics_.push_back(def.make(raw_data_cache_));
    }

    for (const auto &metric : metrics_)
    {
      const std::vector<RawData::Any> &required = metric->required_raw_data();
      std::for_each(required.cbegin(), required.cend(), [&](RawData::Any kind) { add_raw_data_kind(kind); });
    }

    // Determine required raw data to collect
    add_raw_data_kind(RawData::CpuThreads{}); // SystemInfoWindow
    add_raw_data_kind(RawData::CpuInfo{});    // SystemInfoWindow
    add_raw_data_kind(RawData::KernelInfo{}); // SystemInfoWindow
    collect_raw_data();
    collect_single_procs_info();

    // Window initialization
    const size_t bar_plot_row_count = (tui_.get_size().rows - 1) / metrics_.size();
    const size_t bar_plot_col_count = tui_.get_size().cols * BAR_PLOTS_WIDTH_RATIO;
    const size_t right_hand_window_col_count = tui_.get_size().cols - bar_plot_col_count;

    const size_t system_info_window_row_count = 4;
    const auto cpu_threads = raw_data_cache_.get<RawData::CpuThreads>();
    const auto cpu_info = raw_data_cache_.get<RawData::CpuInfo>();
    const auto kernel_info = raw_data_cache_.get<RawData::KernelInfo>();
    if (!cpu_threads || !cpu_info || !kernel_info)
    {
      throw std::runtime_error("failed to get system info");
    }

    // System Info Window
    windows_.push_back(
        std::make_unique<SystemInfoWindow>(
            "tower",
            Rect{0, bar_plot_col_count, system_info_window_row_count, right_hand_window_col_count},
            *cpu_threads,
            *cpu_info,
            *kernel_info));

    // Process List Window
    const size_t process_list_window_row_count = (tui_.get_size().rows - 1) - system_info_window_row_count;
    windows_.push_back(
        std::make_unique<ProcessListWindow>(
            "process list",
            Rect{
                system_info_window_row_count,
                bar_plot_col_count,
                process_list_window_row_count,
                right_hand_window_col_count},
            single_procs_info_));

    // User Input Window
    windows_.push_back(
        std::make_unique<UserInputWindow>(
            "user input",
            Rect{static_cast<size_t>(tui_.get_size().rows - 1), 0, 1, tui_.get_size().cols},
            user_input_buf_));

    // Metric Windows
    size_t row_offset = 0;
    for (size_t i = 0; i < METRIC_DEFINITIONS.size(); ++i)
    {
      const auto &def = METRIC_DEFINITIONS[i];
      const auto &metric = metrics_[i];

      const Rect rect{
          .row_offset = row_offset, .col_offset = 0, .row_count = bar_plot_row_count, .col_count = bar_plot_col_count};

      windows_.push_back(
          std::make_unique<BarPlotWindow>(
              def.name,
              rect,
              def.initial_ymin,
              def.initial_ymax(raw_data_cache_),
              def.dynamic_ymax,
              def.color,
              [format = def.format](double v) { return format(v); },
              metric->rb()));

      row_offset += bar_plot_row_count;
    }
  }

  void run()
  {
    auto now = std::chrono::steady_clock::now();
    auto tick_start = now;
    auto next_input_tick = now;
    auto next_collection_tick = now;

    while (running_)
    {
      now = std::chrono::steady_clock::now();

      if (now >= next_input_tick)
      {
        tick_start = std::chrono::steady_clock::now();
        next_input_tick += std::chrono::milliseconds(USER_INPUT_INTERVAL_MS);

        handle_keyboard_input();

        warn_if_overrun("input", tick_start, USER_INPUT_INTERVAL_MS);
      }

      if (now >= next_collection_tick)
      {
        tick_start = std::chrono::steady_clock::now();
        next_collection_tick += std::chrono::milliseconds(COLLECTION_INTERVAL_MS);

        collect_raw_data();
        collect_single_procs_info();
        update_metrics();
        draw_windows();

        warn_if_overrun("collection", tick_start, COLLECTION_INTERVAL_MS);
      }

      frame_buffer_.draw();
      std::this_thread::sleep_until(std::min(next_input_tick, next_collection_tick));
    }
  };

private:
  bool running_ = true;

  const Tui tui_;

  FrameBuffer frame_buffer_{tui_.get_size().rows, tui_.get_size().cols};

  std::vector<RawData::Any> raw_data_kinds_;
  RawDataCache raw_data_cache_;

  std::vector<std::unique_ptr<Metric>> metrics_;
  std::vector<SingleProcInfo> single_procs_info_;
  ProcessListSortKey proc_list_sort_key_ = ProcessListSortKey::Mem;

  std::vector<std::unique_ptr<Window>> windows_;

  std::optional<int> selected_pid_ = std::nullopt;
  std::string user_input_buf_ = "";

  void warn_if_overrun(std::string_view tick_name, std::chrono::steady_clock::time_point start, int target_ms)
  {
    const auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > std::chrono::milliseconds(target_ms))
    {
      const auto actual_ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
      LOG_WARNING(std::format("{} tick overran (target: {}ms, actual {}ms)", tick_name, target_ms, actual_ms));
    }
  }

  void add_raw_data_kind(RawData::Any kind)
  {
    const bool already_present = std::any_of(
        raw_data_kinds_.cbegin(),
        raw_data_kinds_.cend(),
        [&](const auto &existing) { return existing.index() == kind.index(); });
    if (!already_present)
    {
      raw_data_kinds_.push_back(kind);
    }
  }

  void collect_raw_data()
  {
    for (const auto &kind : raw_data_kinds_)
    {
      std::visit(
          [&](const auto &tag)
          {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (SystemRawData<T>)
            {
              if (auto result = T::collect())
              {
                raw_data_cache_.store(*result);
              }
            }
            else if constexpr (ProcessRawData<T>)
            {
              if (const auto pid = selected_pid_)
              {
                if (auto result = T::collect(*pid))
                {
                  raw_data_cache_.store(*result);
                }
              }
            }
          },
          kind);
    }
  }

  void collect_single_procs_info()
  {
    static std::unordered_map<int, unsigned long> proc_to_previous_jiffies;
    static std::chrono::steady_clock::time_point previous_read = std::chrono::steady_clock::now();

    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    single_procs_info_.clear();
    for (const auto &dir_entry : std::filesystem::directory_iterator("/proc"))
    {
      SingleProcInfo proc_info;

      std::string_view filename = dir_entry.path().c_str();
      filename.remove_prefix(filename.rfind('/') + 1);

      // PID
      auto [ptr, ec] = std::from_chars(filename.begin(), filename.end(), proc_info.pid);
      if (ec != std::errc{} || ptr != filename.data() + filename.size())
      {
        continue;
      }

      // NAME
      std::ifstream file(dir_entry.path() / "comm");
      if (!file)
        continue;
      if (!std::getline(file, proc_info.name))
        continue;

      // MEM
      const auto proc_status = RawData::ProcStatus::collect(proc_info.pid);
      if (!proc_status)
        continue;
      proc_info.mem_usage_kb = proc_status->vm_rss_kb;

      // CPU
      const auto proc_stat = RawData::ProcStat::collect(proc_info.pid);
      if (!proc_stat)
        continue;
      const unsigned long current_jiffies = proc_stat->utime + proc_stat->stime;
      const auto clock_tick = raw_data_cache_.get<RawData::ClockTick>();
      const auto cpu_threads = raw_data_cache_.get<RawData::CpuThreads>();
      if (proc_to_previous_jiffies.contains(proc_info.pid) && clock_tick && cpu_threads)
      {
        const double elapsed = std::chrono::duration<double>(now - previous_read).count();
        const unsigned long delta_jiffies = current_jiffies - proc_to_previous_jiffies[proc_info.pid];
        const double usage = delta_jiffies / (elapsed * clock_tick->value) * 100 / cpu_threads->value;
        proc_info.cpu_usage_pct = usage;
      }

      single_procs_info_.push_back(proc_info);
      proc_to_previous_jiffies[proc_info.pid] = current_jiffies;
    }
    previous_read = now;

    std::sort(
        single_procs_info_.begin(),
        single_procs_info_.end(),
        [this](const SingleProcInfo &a, const SingleProcInfo &b)
        {
          switch (proc_list_sort_key_)
          {
          case ProcessListSortKey::Cpu:
            return a.cpu_usage_pct > b.cpu_usage_pct;
          case ProcessListSortKey::Mem:
            return a.mem_usage_kb > b.mem_usage_kb;
          case ProcessListSortKey::Pid:
            return a.pid < b.pid;
          case ProcessListSortKey::Name:
            return a.name < b.name;
          }
          return false; // unreachable
        });
  }

  void clear_process_raw_data()
  {
    for (const auto &kind : raw_data_kinds_)
    {
      std::visit(
          [&](const auto &tag)
          {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (ProcessRawData<T>)
            {
              raw_data_cache_.reset<T>();
            }
          },
          kind);
    }
  }

  void update_metrics()
  {
    for (auto &metric : metrics_)
    {
      metric->update();
    }
  }

  void draw_windows()
  {
    for (const auto &window : windows_)
    {
      window->draw(frame_buffer_.back_buf());
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
      else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || (c == ' '))
      {
        user_input_buf_ += c;
      }
    }
  }

  bool execute_command(std::string_view input)
  {
    bool success = false;
    const std::string_view cmd = parse_next_token(input);

    if (cmd == "exit")
    {
      running_ = false;
    }
    else if (cmd == "set")
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
          clear_process_raw_data();
          for (auto &metric : metrics_)
          {
            metric->reset();
          }

          success = true;
        }
      }
      else if (subcmd == "procsort")
      {
        const std::string_view arg = parse_next_token(input);
        success = true;
        if (arg == "cpu")
        {
          proc_list_sort_key_ = ProcessListSortKey::Cpu;
        }
        else if (arg == "mem")
        {
          proc_list_sort_key_ = ProcessListSortKey::Mem;
        }
        else if (arg == "pid")
        {
          proc_list_sort_key_ = ProcessListSortKey::Pid;
        }
        else if (arg == "name")
        {
          proc_list_sort_key_ = ProcessListSortKey::Name;
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
};
