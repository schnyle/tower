#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <unistd.h>

#include "loadavg.hpp"
#include "logger.hpp"
#include "meminfo.hpp"
#include "stat.hpp"
#include "tui.hpp"

int main(void)
{
  LOG_INFO("starting tower");
  LOG_DEBUG("starting tower in DEBUG mode");

  tui_enter();
  tui_clear();

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
    if (const auto meminfo = get_proc_meminfo())
    {
      meminfo->print();
      printf("\n");
    };

    if (const auto loadavg = get_proc_loadavg())
    {
      loadavg->print();
      printf("\n");
    }

    if (const auto stat = get_proc_stat())
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
    double duration = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
    // LOG_INFO("spent ", (int)duration, " μs reading /proc");

    next += std::chrono::milliseconds(1000);
    std::this_thread::sleep_until(next);

    tui_clear();
  }
}
