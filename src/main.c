#include <termios.h>
#include <unistd.h>

#include "tower/config.h"
#include "tower/log.h"
#include "tower/metric.h"
#include "tower/tui/tui.h"

int main()
{
  init_config();
  LOG_INFO("configuration and logger initialized");

  MemInfo meminfo;

  struct termios original_termios;
  tui_get_termios(&original_termios);

  tui_enter();
  tui_clear();

  while (1)
  {
    get_proc_meminfo(&meminfo);
    draw_tui(&meminfo);

    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
      break;

    sleep(1);
  }

  return 0;
}
