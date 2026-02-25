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

void draw_rect(int x, int y, int w, int h)
{
  // top border
  front_set(x, y, "┌");
  for (int i = 1; i < w - 1; ++i)
    front_set(x + i, y, "─");
  front_set(x + w - 1, y, "┐");

  // sides
  for (int i = 1; i < h - 1; ++i)
  {
    front_set(x, y + i, "│");
    front_set(x + w - 1, y + i, "│");
  }

  // bottom border
  front_set(x, y + h - 1, "└");
  for (int i = 1; i < w - 1; ++i)
    front_set(x + i, y + h - 1, "─");
  front_set(x + w - 1, y + h - 1, "┘");
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

  draw_rect(1, 1, 5, 5);

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

  // printf("MemInfo:\r\n");
  // printf("\tTotal: %ld kB\r\n", meminfo->total_kb);
  // printf("\tFree: %ld kB\r\n", meminfo->free_kb);
  // printf("\tAvailable: %ld kB\r\n", meminfo->available_kb);
  // printf("\tSwap Total: %ld kB\r\n", meminfo->swap_total_kb);
  // printf("\tSwap Free: %ld kB\r\n", meminfo->swap_free_kb);
  // printf("\n");
  // printf("\r\nPress 'q' to quit\r\n");
  fflush(stdout);
};
