#ifndef RECT_H
#define RECT_H

typedef struct
{
  int xmin;
  int ymin;
  int width;
  int height;
} Rect;

void rect_draw(const Rect, const char *);

void rect_draw_text(const Rect, const char *s);

#endif
