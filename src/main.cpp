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

  tui_enter();
  tui_clear();

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
      stat->print();
      printf("\n");
    }
    double duration = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
    // LOG_INFO("spent ", (int)duration, " μs reading /proc");

    next += std::chrono::milliseconds(1000);
    std::this_thread::sleep_until(next);

    tui_clear();
  }
}
