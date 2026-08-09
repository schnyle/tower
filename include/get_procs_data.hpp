#pragma once

#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "collect.hpp"
#include "logger.hpp"
#include "parsers/proc_stat.hpp"
#include "parsers/proc_status.hpp"

struct ProcData
{
  int pid;
  std::string name;
  long long mem_usage_kb;
  long unsigned utime;
  long unsigned stime;
  double cpu_usage_pct = 0; // computed in poc_tui
};

inline std::vector<ProcData> get_procs_data()
{
  int pid;
  std::string name;
  std::vector<ProcData> result;

  for (const auto &entry : std::filesystem::directory_iterator("/proc"))
  {
    std::string_view filename = entry.path().c_str();
    filename.remove_prefix(filename.rfind('/') + 1);

    // PID
    auto [ptr, ec] = std::from_chars(filename.begin(), filename.end(), pid);
    if (ec != std::errc{} || ptr != filename.data() + filename.size())
    {
      continue;
    }

    // NAME
    std::ifstream file(entry.path() / "comm");
    if (!file)
    {
      continue;
    }

    if (!std::getline(file, name))
    {
      continue;
    }

    // MEM KB
    const auto status = collect<ProcStatusParser>(pid);
    if (!status)
    {
      continue;
    }

    // CPU JIFFIES
    const auto stat = collect<ProcStatParser>(pid);
    if (!stat)
    {
      continue;
    }

    result.push_back(
        {.pid = pid,
         .name = name,
         .mem_usage_kb = status->value.vm_rss_kb,
         .utime = stat->value.utime,
         .stime = stat->value.stime});
  }

  return result;
};
