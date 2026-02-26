#include <string.h>

#include "tower/tui/rect.h"
#include "tower/tui/tui.h"

void rect_draw(const Rect rect, const char *name)
{
  // top border
  front_set(rect.xmin, rect.ymin, "┌");
  for (int i = 1; i < rect.width - 1; ++i)
    front_set(rect.xmin + i, rect.ymin, "─");
  front_set(rect.xmin + rect.width - 1, rect.ymin, "┐");

  // sides
  for (int i = 1; i < rect.height - 1; ++i)
  {
    front_set(rect.xmin, rect.ymin + i, "│");
    front_set(rect.xmin + rect.width - 1, rect.ymin + i, "│");
  }

  // bottom border
  front_set(rect.xmin, rect.ymin + rect.height - 1, "└");
  for (int i = 1; i < rect.width - 1; ++i)
    front_set(rect.xmin + i, rect.ymin + rect.height - 1, "─");
  front_set(rect.xmin + rect.width - 1, rect.ymin + rect.height - 1, "┘");

  // name
  if (!name || name[0] == '\0')
    return;

  const int len = strlen(name);
  if (len + 2 > rect.width - 2)
    return;

  front_set(rect.xmin + 1, rect.ymin, " ");
  for (int i = 0; i < len; ++i)
  {
    char buf[2] = {name[i], '\0'};
    front_set(rect.xmin + i + 2, rect.ymin, buf);
  }
  front_set(rect.xmin + len + 2, rect.ymin, " ");
}

void rect_draw_text(const Rect rect, const char *s)
{
  const int len = strlen(s);
  const int y_mid = rect.ymin + rect.height / 2;
  const int x_start = rect.xmin + rect.width / 2 - len / 2;

  for (int i = 0; i < len; ++i)
  {
    char buf[2] = {s[i], '\0'};
    front_set(x_start + i, y_mid, buf);
  }
}
