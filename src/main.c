#include <termios.h>
#include <unistd.h>

#include "tower/config.h"
#include "tower/log.h"
#include "tower/metric.h"
#include "tower/ring_buffer.h"
#include "tower/tui/tui.h"

#define METRIC_HISTORY_SIZE 3

int main()
{
  init_config();
  LOG_INFO("configuration and logger initialized");

  MemInfo meminfo;

  RingBuffer *mem_free_kb = rb_create(METRIC_HISTORY_SIZE, sizeof(long));
  if (mem_free_kb == NULL)
  {
    LOG_ERROR("failed to initialize metrics data");
    return 1;
  }

  tui_enter();
  tui_clear();

  while (1)
  {
    get_proc_meminfo(&meminfo);
    draw_tui(&meminfo);

    rb_push(mem_free_kb, &meminfo.free_kb, sizeof(meminfo.free_kb));
    LOG_INFO("");
    LOG_INFO("count: %d\n", mem_free_kb->count);
    // for (int i = 0; i < mem_free_kb->count; ++i)
    // {
    //   LOG_INFO("%ul\n", mem_free_kb->data[mem_free_kb->head + i] % mem_free_kb->size);
    //   // LOG_INFO(
    //   //     "%f\n",
    //   //     mem_free_kb.data[(mem_free_kb.head + 1 - mem_free_kb.count + i + RING_BUFFER_SIZE) % RING_BUFFER_SIZE]);
    // }
    LOG_INFO("");

    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
      break;

    sleep(1);
  }

  return 0;
}
