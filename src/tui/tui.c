#include <stdio.h>
#include <stdlib.h>
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

void tui_get_termios(struct termios *termios) { tcgetattr(STDIN_FILENO, termios); }

void tui_enter(void)
{
  tcgetattr(STDIN_FILENO, &original_terminal);
  atexit(tui_exit);

  struct termios terminal;
  tcgetattr(STDIN_FILENO, &terminal);

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
};

void tui_clear(void)
{
  fputs(CURSOR_HIDE, stdout);
  fputs(CLEAR_SCREEN, stdout);
}

void draw_tui(const MemInfo *const meminfo)
{
  fputs(CURSOR_HOME, stdout); // begin drawing at top of screen

  printf("MemInfo:\r\n");
  printf("\tTotal: %ld kB\r\n", meminfo->total_kb);
  printf("\tFree: %ld kB\r\n", meminfo->free_kb);
  printf("\tAvailable: %ld kB\r\n", meminfo->available_kb);
  printf("\tSwap Total: %ld kB\r\n", meminfo->swap_total_kb);
  printf("\tSwap Free: %ld kB\r\n", meminfo->swap_free_kb);
  printf("\n");
  printf("\r\nPress 'q' to quit\r\n");

  fflush(stdout);
};
