#include <chrono>
#include <cstdio>
#include <optional>
#include <thread>
#include <unistd.h>

#include "meminfo.hpp"
#include "tui.hpp"

int main(void)
{
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

    if (const auto meminfo = get_proc_meminfo())
    {
      meminfo->print();
      printf("\n");
    };

    next += std::chrono::seconds(1);
    std::this_thread::sleep_until(next);

    tui_clear();
  }
}
