#include "tower/tui/bar.h"
#include "tower/log.h"
#include "tower/ring_buffer.h"
#include "tower/tui/rect.h"
#include "tower/tui/tui.h"

void bar_draw(const Bar bar, const char *name)
{
  const Rect rect = bar.rect;
  rect_draw(rect, name);

  const int plot_width = rect.width - 2;
  const int plot_height = rect.height - 2;
  const int plot_xmin = rect.xmin + 1;
  const int plot_ymin = rect.ymin + 1;

  const int ymid = plot_height / 2 + plot_ymin;

  const RingBuffer *ring_buffer = bar.rb;

  const int elements = ring_buffer->count >= plot_width ? plot_width : ring_buffer->count;
  const int xstart = plot_xmin + plot_width - elements;

  for (int i = 0; i < elements; ++i)
  {
    double value = -1;
    const void *raw_value = rb_get(ring_buffer, i);
    if (raw_value == NULL)
    {
      LOG_ERROR("whoops");
      continue;
    }

    // LOG_INFO("%ul\n", (long *)raw_value);
    switch (ring_buffer->data_type)
    {
    case DATA_TYPE_DOUBLE:
      value = *(double *)raw_value;
      break;
    case DATA_TYPE_LONG:
      value = *(long *)raw_value;
      break;
    }

    if (i == elements - 1)
    {
      LOG_INFO("%ul\n", *(long *)raw_value);
    }

    double scaled_value = value / bar.data_max;
    scaled_value *= plot_height;
    // LOG_INFO("%f\n", scaled_value);
    scaled_value += plot_ymin;

    const int yval = plot_ymin + (plot_height - scaled_value);

    for (int j = yval; j < plot_height + plot_ymin; ++j)
    {
      front_set(xstart + i, j, "$");
    }
  }
};
