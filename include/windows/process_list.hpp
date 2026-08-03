#pragma once

#include <string>

#include "build_window_frame.hpp"
#include "canvas.hpp"
#include "format_utils.hpp"
#include "get_procs_data.hpp"
#include "windows/window.hpp"

static constexpr size_t PID_WIDTH = 7;
static constexpr size_t NAME_WIDTH = 16;
static constexpr size_t MEM_WIDTH = 5;
static constexpr size_t CPU_WIDTH = 5;

class ProcessListWindow : public Window
{
public:
  ProcessListWindow(std::string name, Rect rect, std::vector<ProcData> &processes_data)
      : Window(std::move(name), rect), processes_data_(processes_data)
  {
  }

  void draw(Canvas &canvas) const override
  {
    build_window_frame(canvas, rect().row_offset, rect().col_offset, rect().row_count, rect().col_count, name());

    const Rect process_table_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 1, rect().row_count - 2, rect().col_count - 2};
    build_processes_table(canvas, process_table_rect);
  }

private:
  const std::vector<ProcData> &processes_data_;

  void build_processes_table(Canvas &canvas, const Rect rect) const
  {
    for (size_t i = 0; i < rect.row_count; ++i)
    {
      canvas.copy_n(rect.row_offset + i, rect.col_offset, std::string(rect.col_count, ' '));
    }

    const std::string header = std::format(
        "{:>{}} {:<{}} {:>{}} {:>{}}", "PID", PID_WIDTH, "NAME", NAME_WIDTH, "MEM", MEM_WIDTH, "CPU", CPU_WIDTH);
    canvas.copy_n(rect.row_offset, rect.col_offset, header);

    size_t i = 0;
    while (i < processes_data_.size() && i < rect.row_count - 1)
    {
      const ProcData &data_item = processes_data_[i];

      const std::string pid = std::to_string(data_item.pid);
      canvas.copy_n(rect.row_offset + i + 1, rect.col_offset, pid, std::min(pid.size(), PID_WIDTH));

      const std::string_view name = data_item.name;
      canvas.copy_n(rect.row_offset + i + 1, rect.col_offset + PID_WIDTH + 1, name, std::min(name.size(), NAME_WIDTH));

      const std::string mem_usage_string = format_memory_size(data_item.mem_usage_kb);
      canvas.copy_n(
          rect.row_offset + i + 1,
          rect.col_offset + PID_WIDTH + 1 + NAME_WIDTH + 1,
          mem_usage_string,
          std::min(mem_usage_string.size(), MEM_WIDTH));

      const std::string cpu_usage_string = format_percent(data_item.cpu_usage_pct);
      canvas.copy_n(
          rect.row_offset + i + 1,
          rect.col_offset + PID_WIDTH + 1 + NAME_WIDTH + 1 + MEM_WIDTH + 1,
          cpu_usage_string,
          std::min(cpu_usage_string.size(), CPU_WIDTH));

      ++i;
    }
  }
};
