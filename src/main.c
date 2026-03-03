#include <termios.h>
#include <unistd.h>

#include "tower/config.h"
#include "tower/log.h"
#include "tower/metric.h"
#include "tower/ring_buffer.h"
#include "tower/tui/tui.h"

#define METRIC_HISTORY_SIZE 256

int main()
{
  init_config();
  LOG_INFO("configuration and logger initialized");

  MemInfo meminfo;

  RingBuffer *mem_free_kb = rb_create(METRIC_HISTORY_SIZE, sizeof(meminfo.free_kb), DATA_TYPE_LONG);
  if (mem_free_kb == NULL)
  {
    LOG_ERROR("failed to initialize metrics data");
    return 1;
  }

  tui_enter();
  tui_clear();

  while (1)
  {
    // collect metrics
    get_proc_meminfo(&meminfo);

    // update internal data structures
    rb_push(mem_free_kb, &meminfo.available_kb, sizeof(meminfo.free_kb));

    // draw TUI
    draw_tui(mem_free_kb);

    // collect user input
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
      break;

    sleep(1);
  }

  rb_destroy(mem_free_kb);

  return 0;
}
