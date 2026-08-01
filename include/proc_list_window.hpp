#pragma once

#include <algorithm>
#include <format>
#include <string>
#include <vector>

#include "build_window_frame.hpp"
#include "canvas.hpp"
#include "get_procs_data.hpp"
#include "logger.hpp"

static constexpr size_t PID_WIDTH = 7;
static constexpr size_t NAME_WIDTH = 16;

struct ProcListWindow
{
  size_t row_offset, col_offset, row_count, col_count;
  std::string title;
  std::vector<ProcData> &data;
};

inline std::ostream &operator<<(std::ostream &os, const ProcListWindow &c)
{
  return os << "ProcListWindow{row_offset: " << c.row_offset << ", col_offset: " << c.col_offset
            << " row_count: " << c.row_count << ", col_count: " << c.col_count << "}";
}

inline void draw_processes_table(
    Canvas &canvas,
    const size_t row_offset,
    const size_t col_offset,
    const size_t row_count,
    const size_t col_count,
    const std::vector<ProcData> &data)
{
  const std::string header = std::format("{:<{}} {:<{}}", "PID", PID_WIDTH, "NAME", NAME_WIDTH);
  canvas.copy_n(row_offset, col_offset, header);

  size_t i = 0;
  while (i < data.size() && i < row_count - 1)
  {
    const ProcData data_item = data[i];

    const std::string pid = std::to_string(data_item.pid);
    canvas.copy_n(row_offset + i + 1, col_offset, pid, std::min(pid.size(), PID_WIDTH));

    const std::string_view name = data_item.name;
    canvas.copy_n(row_offset + i + 1, col_offset + PID_WIDTH + 1, name, std::min(name.size(), NAME_WIDTH));
    ++i;
  }
}

inline void draw_proc_window(Canvas &canvas, const ProcListWindow &w)
{
  static const std::string heading = w.title;
  build_window_frame(canvas, w.row_offset, w.col_offset, w.row_count, w.col_count, heading);
  draw_processes_table(canvas, w.row_offset + 1, w.col_offset + 1, w.row_count - 2, w.col_count - 2, w.data);
}
