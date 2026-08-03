#pragma once

#include <string>

#include "build_process_list.hpp"
#include "build_window_frame.hpp"
#include "canvas.hpp"
#include "get_procs_data.hpp"
#include "windows/window.hpp"

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
    build_processes_table(canvas, process_table_rect, processes_data_);
  }

private:
  const std::vector<ProcData> &processes_data_;
};
