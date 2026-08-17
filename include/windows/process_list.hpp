#pragma once

#include <string>

#include "canvas.hpp"
#include "drawing/draw_process_list.hpp"
#include "drawing/draw_window_frame.hpp"
#include "single_proc_info.hpp"
#include "windows/window.hpp"

enum class ProcessListSortKey
{
  Cpu,
  Mem,
  Pid,
  Name
};

class ProcessListWindow : public Window
{
public:
  ProcessListWindow(std::string name, Rect rect, const std::vector<SingleProcInfo> &processes_data)
      : Window(std::move(name), rect), single_procs_info_(processes_data)
  {
  }

  static size_t width() { return content_width() + 2; }

  void draw(Canvas &canvas) const override
  {
    draw_window_frame(canvas, rect(), name());

    const Rect process_table_rect = Rect{
        rect().row_offset + 1, rect().col_offset + 1, rect().row_count - 2, rect().col_count - 2};
    draw_processes_table(canvas, process_table_rect, single_procs_info_);
  }

private:
  const std::vector<SingleProcInfo> &single_procs_info_;
};
