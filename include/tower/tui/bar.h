#ifndef TUI_BAR_H
#define TUI_BAR_H

#include "tower/ring_buffer.h"
#include "tower/tui/rect.h"

typedef struct
{
  const Rect rect;
  const double data_max;
  const RingBuffer *rb;
} Bar;

void bar_draw(const Bar bar, const char *name);

#endif
