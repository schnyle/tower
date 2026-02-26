#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "tower/metric.h"
#include "tower/tui/tui.h"

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define ALT_SCREEN_ON "\033[?1049h"
#define ALT_SCREEN_OFF "\033[?1049l"

static struct termios original_terminal;

typedef struct
{
  char bytes[4];
  uint8_t color;
} Cell;

typedef struct
{
  Cell *front;
  Cell *back;
  char *render_buf;
  size_t render_buf_size;
  int rows;
  int cols;
} ScreenBuffer;

static ScreenBuffer buffer;

void front_set(int x, int y, const char *ch)
{
  Cell *cell = &buffer.front[y * buffer.cols + x];
  memcpy(cell->bytes, ch, 4);
}

void move_cursor(int row, int col) { printf("\033[%d;%dH", row, col); }

void get_terminal_size(int *rows, int *cols)
{
  struct winsize ws;
  ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
  *rows = ws.ws_row;
  *cols = ws.ws_col;
}

typedef struct
{
  int x;
  int y;
  int w;
  int h;
} Rect;

void draw_rect(const Rect rect, const char *name)
{
  // top border
  front_set(rect.x, rect.y, "┌");
  for (int i = 1; i < rect.w - 1; ++i)
    front_set(rect.x + i, rect.y, "─");
  front_set(rect.x + rect.w - 1, rect.y, "┐");

  // sides
  for (int i = 1; i < rect.h - 1; ++i)
  {
    front_set(rect.x, rect.y + i, "│");
    front_set(rect.x + rect.w - 1, rect.y + i, "│");
  }

  // bottom border
  front_set(rect.x, rect.y + rect.h - 1, "└");
  for (int i = 1; i < rect.w - 1; ++i)
    front_set(rect.x + i, rect.y + rect.h - 1, "─");
  front_set(rect.x + rect.w - 1, rect.y + rect.h - 1, "┘");

  // name
  if (!name || name[0] == '\0')
    return;

  const int len = strlen(name);
  if (len + 2 > rect.w - 2)
    return;

  front_set(rect.x + 1, rect.y, " ");
  for (int i = 0; i < len; ++i)
  {
    char buf[2] = {name[i], '\0'};
    front_set(rect.x + i + 2, rect.y, buf);
  }
  front_set(rect.x + len + 2, rect.y, " ");
}

void draw_rect_text(const Rect rect, const char *s)
{
  const int len = strlen(s);
  const int y_mid = rect.y + rect.h / 2;
  const int x_start = rect.x + rect.w / 2 - len / 2;

  for (int i = 0; i < len; ++i)
  {
    char buf[2] = {s[i], '\0'};
    front_set(x_start + i, y_mid, buf);
  }
}

void tui_enter(void)
{
  tcgetattr(STDIN_FILENO, &original_terminal);
  atexit(tui_exit);

  struct termios terminal;
  tcgetattr(STDIN_FILENO, &terminal);

  int rows, cols;
  get_terminal_size(&rows, &cols);
  buffer.rows = rows;
  buffer.cols = cols;
  buffer.front = calloc(cols * rows, sizeof(Cell));
  buffer.back = calloc(cols * rows, sizeof(Cell));
  buffer.render_buf_size = rows * 4 + cols * 4 + rows * 2 + 8;
  buffer.render_buf = malloc(buffer.render_buf_size * 4);

  cfmakeraw(&terminal);
  terminal.c_cc[VMIN] = 0;
  terminal.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal);

  fputs(ALT_SCREEN_ON, stdout);
  fflush(stdout);
};

void tui_exit()
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal);
  fputs(ALT_SCREEN_OFF, stdout);
  fputs(CURSOR_SHOW, stdout);
  fflush(stdout);
};

void tui_clear(void)
{
  fputs(CURSOR_HIDE, stdout);
  fputs(CLEAR_SCREEN, stdout);
  fflush(stdout);
}

void draw_tui(const MemInfo *const meminfo)
{
  fputs(CURSOR_HOME, stdout); // begin drawing at top of screen

  const Rect text_rect = {1, 1, 100, 40};
  draw_rect(text_rect, "Ayooo");

  char value_s[32];
  sprintf(value_s, "Mem Available %ld kB", meminfo->available_kb);
  draw_rect_text(text_rect, value_s);

  size_t offset = 0;

  for (int row = 0; row < buffer.rows; ++row)
  {
    for (int col = 0; col < buffer.cols; ++col)
    {
      Cell *cell = &buffer.front[row * buffer.cols + col];
      if (cell->bytes[0] == '\0')
      {
        buffer.render_buf[offset++] = ' ';
      }
      else
      {
        memcpy(buffer.render_buf + offset, cell->bytes, strnlen(cell->bytes, 4));
        offset += strnlen(cell->bytes, 4);
      }
    }
    buffer.render_buf[offset++] = '\r';
    buffer.render_buf[offset++] = '\n';
  }

  write(STDOUT_FILENO, buffer.render_buf, offset);
  fflush(stdout);
};
