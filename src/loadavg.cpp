#include <charconv>
#include <fstream>
#include <iostream>
#include <string>

#include "loadavg.hpp"

std::optional<LoadAvg> get_proc_loadavg()
{
  static const std::string PROC_LOADAVG_PATH = "/proc/loadavg";

  std::ifstream file(PROC_LOADAVG_PATH);
  if (!file)
  {
    std::cerr << "failed to open file " << PROC_LOADAVG_PATH << "\n";
    return std::nullopt;
  }

  double load_avg_1_min, load_avg_5_min, load_avg_15_min;
  int runnable_procs, total_procs, last_pid_created;
  std::string proc_str;

  if (!(file >> load_avg_1_min >> load_avg_5_min >> load_avg_15_min >> proc_str >> last_pid_created))
  {
    return std::nullopt;
  }

  const size_t pos = proc_str.find('/');
  if (pos == std::string::npos)
  {
    return std::nullopt;
  }

  if (std::from_chars(proc_str.data(), proc_str.data() + pos, runnable_procs).ec != std::errc{})
  {
    return std::nullopt;
  }

  if (std::from_chars(proc_str.data() + pos + 1, proc_str.data() + proc_str.size(), total_procs).ec != std::errc{})
  {
    return std::nullopt;
  }

  LoadAvg loadavg;
  loadavg.load_avg_1_min = load_avg_1_min;
  loadavg.load_avg_5_min = load_avg_5_min;
  loadavg.load_avg_15_min = load_avg_15_min;
  loadavg.runnable_procs = runnable_procs;
  loadavg.total_procs = total_procs;
  loadavg.last_pid_created = last_pid_created;

  return loadavg;
}
