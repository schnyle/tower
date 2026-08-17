#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <variant>

#include "format_utils.hpp"
#include "frame_buffer.hpp"
#include "logger.hpp"
#include "metric.hpp"
#include "raw_data/file.hpp"
#include "raw_data/raw_data.hpp"
#include "raw_data/raw_data_cache.hpp"
#include "single_proc_info.hpp"
#include "tui.hpp"
#include "windows/bar_plot/double_bar_plot.hpp"
#include "windows/bar_plot/single_bar_plot.hpp"
#include "windows/process_list.hpp"
#include "windows/system_info.hpp"
#include "windows/user_input_window.hpp"
#include "windows/window.hpp"

// config
static constexpr int USER_INPUT_INTERVAL_MS = 16;
static constexpr int COLLECTION_INTERVAL_MS = 500;
static constexpr int DATA_POINTS = 50000;

static constexpr double BAR_PLOTS_WIDTH_RATIO = 0.65;

struct SingleBarPlotMetricDefinition
{
  std::string name;
  Color color;
  double initial_ymin;
  std::function<double(const RawDataCache &)> initial_ymax;
  bool dynamic_ymax;
  bool never_empty;
  std::function<std::string(double)> format;
  std::function<std::unique_ptr<Metric>(const RawDataCache &)> make;
};

struct DoubleBarPlotMetricDefinition
{
  std::string name;
  Color color_top;
  Color color_bottom;
  double initial_ymin;
  std::function<double(const RawDataCache &)> initial_ymax;
  bool dynamic_ymax;
  bool never_empty;
  std::function<std::string(double)> format;
  std::function<std::unique_ptr<Metric>(const RawDataCache &)> make_top;
  std::function<std::unique_ptr<Metric>(const RawDataCache &)> make_bottom;
};

using PlotDefinition = std::variant<SingleBarPlotMetricDefinition, DoubleBarPlotMetricDefinition>;

static const std::vector<PlotDefinition> METRIC_DEFINITIONS = {
    SingleBarPlotMetricDefinition{
        .name = "CPU Load",
        .color = Color::red(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = false,
        .never_empty = true,
        .format = [](const double v) { return std::format("{:.1f}%", v); },
        .make = make_cpu_load_metric,
    },
    SingleBarPlotMetricDefinition{
        .name = "Available Memory",
        .color = Color::green(),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &cache) { return cache.get<RawData::MemInfo>()->mem_total_kb; },
        .dynamic_ymax = false,
        .never_empty = false,
        .format = [](const double v) { return format_bytes_iec(v / 1024); },
        .make = make_gauge_metric<RawData::MemInfo, &RawData::MemInfo::mem_available_kb>,
    },
    SingleBarPlotMetricDefinition{
        .name = "Major Page Faults",
        .color = Color(100, 50, 200),
        .initial_ymin = 0.,
        .initial_ymax = [](const RawDataCache &) { return 100.; },
        .dynamic_ymax = true,
        .never_empty = false,
        .format = [](const double v) { return std::format("{:.1f} per sec", v); },
        .make = make_delta_rate_metric<RawData::VmStat, &RawData::VmStat::pgmajfault>,
    },
    DoubleBarPlotMetricDefinition{
        .name = "Network",
        .color_top = Color::blue(),
        .color_bottom = Color::purple(),
        .initial_ymin = 0,
        .initial_ymax = [](const RawDataCache &) { return 10000.; },
        .dynamic_ymax = true,
        .never_empty = true,
        .format = [](const double v) { return std::format("{}/s", format_bytes_si(v)); },
        .make_top = make_delta_rate_metric<RawData::NetDev, &RawData::NetDev::rx_bytes>,
        .make_bottom = make_delta_rate_metric<RawData::NetDev, &RawData::NetDev::tx_bytes>,
    },
    // {
    //     .name = "Process Major Page Faults",
    //     .color = Color(14, 27, 19),
    //     .initial_ymin = 0.,
    //     .initial_ymax = [](const RawDataCache &) { return 100.; },
    //     .dynamic_ymax = true,
    //     .format = [](const double v) { return std::format("{:.1f} per sec", v); },
    //     .make = make_delta_rate_metric<RawData::ProcStat, &RawData::ProcStat::majflt>,
    // },
    // {
    //     .name = "Process Memory Usage",
    //     .color = Color(47, 200, 150),
    //     .initial_ymin = 0.,
    //     .initial_ymax = [](const RawDataCache &cache) { return cache.get<RawData::MemInfo>()->mem_total_kb; },
    //     .dynamic_ymax = false,
    //     .format = [](const double v) { return format_bytes_iec(v / 1024); },
    //     .make = make_gauge_metric<RawData::ProcStatus, &RawData::ProcStatus::vm_rss_kb>,
    // },
    // {
    //     .name = "Process CPU Usage",
    //     .color = Color(150, 47, 120),
    //     .initial_ymin = 0.,
    //     .initial_ymax = [](const RawDataCache &) { return 100.; },
    //     .dynamic_ymax = false,
    //     .format = [](const double v) { return std::format("{:.1f}%", v); },
    //     .make = make_proc_cpu_usage_metric,
    // },
};

class PocTui
{
public:
  PocTui()
  {
    LOG_INFO("starting tower with terminal size: ", tui_.get_size().rows, " rows x ", tui_.get_size().cols, " col");
    initialize_data();
    initialize_layout();
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

      const auto &[term_rows, term_cols] = tui_.get_size();
      if (term_rows != current_term_rows || term_cols != current_term_cols)
      {
        LOG_INFO("window shape change detected, re-initializing TUI layout");
        current_term_rows = term_rows;
        current_term_cols = term_cols;
        initialize_layout();
      }

      if (now >= next_input_tick)
      {
        tick_start = std::chrono::steady_clock::now();
        next_input_tick += std::chrono::milliseconds(USER_INPUT_INTERVAL_MS);

        handle_keyboard_input();
        user_input_window_->draw(frame_buffer_->back_buf());

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

      frame_buffer_->draw();
      std::this_thread::sleep_until(std::min(next_input_tick, next_collection_tick));
    }
  };

private:
  bool running_ = true;

  Tui tui_;
  size_t current_term_rows = tui_.get_size().rows;
  size_t current_term_cols = tui_.get_size().cols;

  std::unique_ptr<FrameBuffer> frame_buffer_;

  std::vector<RawData::Any> raw_data_kinds_;
  RawDataCache raw_data_cache_;

  std::vector<std::unique_ptr<Metric>> metrics_;
  UserInputWindow *user_input_window_ = nullptr;
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

  void initialize_data()
  {
    for (const auto &definition : METRIC_DEFINITIONS)
    {
      std::visit(
          [&](const auto &def)
          {
            using T = std::decay_t<decltype(def)>;

            if constexpr (std::is_same_v<T, SingleBarPlotMetricDefinition>)
            {
              metrics_.push_back(def.make(raw_data_cache_));
            }
            else if constexpr (std::is_same_v<T, DoubleBarPlotMetricDefinition>)
            {
              metrics_.push_back(def.make_top(raw_data_cache_));
              metrics_.push_back(def.make_bottom(raw_data_cache_));
            }
          },
          definition);
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
    add_raw_data_kind(RawData::ClockTick{});  // ProcessList
    collect_raw_data();
    collect_single_procs_info();
  }

  void initialize_layout()
  {
    windows_.clear();
    tui_.clear();

    const auto &[term_rows, term_cols] = tui_.get_size();

    // FrameBuffer
    frame_buffer_ = std::make_unique<FrameBuffer>(term_rows, term_cols);

    // Window initialization
    const size_t bar_plot_row_count = (term_rows - 1) / METRIC_DEFINITIONS.size();
    const size_t bar_plot_col_count = term_cols * BAR_PLOTS_WIDTH_RATIO;
    const size_t right_hand_window_col_count = term_cols - bar_plot_col_count;

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
    const size_t process_list_window_row_count = (term_rows - 1) - system_info_window_row_count;
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
    auto user_input_window = std::make_unique<UserInputWindow>(
        "user input", Rect{static_cast<size_t>(term_rows - 1), 0, 1, term_cols}, user_input_buf_);
    user_input_window_ = user_input_window.get();
    windows_.push_back(std::move(user_input_window));

    // Bar Plot Windows
    size_t row_offset = 0;
    size_t metric_offset = 0;
    for (const auto &definition : METRIC_DEFINITIONS)
    {
      const Rect rect{
          .row_offset = row_offset, .col_offset = 0, .row_count = bar_plot_row_count, .col_count = bar_plot_col_count};

      std::visit(
          [&](const auto &def)
          {
            using T = std::decay_t<decltype(def)>;

            if constexpr (std::is_same_v<T, SingleBarPlotMetricDefinition>)
            {
              windows_.push_back(
                  std::make_unique<SingleBarPlotWindow>(
                      def.name,
                      rect,
                      def.initial_ymin,
                      def.initial_ymax(raw_data_cache_),
                      def.dynamic_ymax,
                      def.never_empty,
                      def.color,
                      [format = def.format](double v) { return format(v); },
                      metrics_[metric_offset]->rb()));
              metric_offset += 1;
            }
            else if constexpr (std::is_same_v<T, DoubleBarPlotMetricDefinition>)
            {
              windows_.push_back(
                  std::make_unique<DoubleBarPlotWindow>(
                      def.name,
                      rect,
                      def.initial_ymin,
                      def.initial_ymax(raw_data_cache_),
                      def.dynamic_ymax,
                      def.never_empty,
                      def.color_top,
                      def.color_bottom,
                      [format = def.format](double v) { return format(v); },
                      metrics_[metric_offset]->rb(),
                      metrics_[metric_offset + 1]->rb()));
              metric_offset += 2;
            }
          },
          definition);

      row_offset += bar_plot_row_count;
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
      window->draw(frame_buffer_->back_buf());
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
