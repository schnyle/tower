#pragma once

#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <unistd.h>

#include "collect.hpp"
#include "logger.hpp"
#include "parsers/loadavg.hpp"
#include "parsers/meminfo.hpp"
#include "parsers/net_dev.hpp"
#include "parsers/stat.hpp"
#include "tui.hpp"

inline void poc_basic_text(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  Tui tui;

  Stat last_stat;

  std::chrono::time_point next = std::chrono::steady_clock::now();
  while (true)
  {
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
    {
      break;
    }

    std::chrono::time_point start = std::chrono::steady_clock::now();
    if (const auto meminfo = collect<MemInfoParser>("/proc/meminfo"))
    {
      meminfo->print();
      printf("\n");
    };

    if (const auto loadavg = collect<LoadAvgParser>("/proc/loadavg"))
    {
      loadavg->print();
      printf("\n");
    }

    if (const auto stat = collect<StatParser>("/proc/stat"))
    {
      CpuTimes delta = stat->cpu_times - last_stat.cpu_times;
      long busy_time = delta.user + delta.nice + delta.system + delta.irq + delta.softirq + delta.steal;
      long total_time = busy_time + delta.idle + delta.iowait;
      double usage = static_cast<double>(busy_time) / static_cast<double>(total_time);
      printf("CPU Load (average): %.0f%%\n\r", usage * 100);

      stat->print();
      printf("\n");

      last_stat = *stat;
    }

    if (const auto net_dev = collect<NetDevParser>("/proc/net/dev"))
    {
      net_dev->print();
      printf("\n");
    }

    double duration = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
    LOG_INFO("spent ", (int)duration, " μs reading /proc");

    next += std::chrono::milliseconds(1000);
    std::this_thread::sleep_until(next);

    tui.clear();
  }
}
