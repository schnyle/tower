#include "tui.hpp"

#include <cstdio>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define ALT_SCREEN_ON "\033[?1049h"
#define ALT_SCREEN_OFF "\033[?1049l"

static struct termios original_termios;

static void tui_exit(void)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
  fputs(ALT_SCREEN_OFF, stdout);
  fputs(CURSOR_SHOW, stdout);
  fflush(stdout);
}

void tui_clear(void)
{
  fputs(CURSOR_HOME, stdout);
  fputs(CURSOR_HIDE, stdout);
  fputs(CLEAR_SCREEN, stdout);
  fflush(stdout);
}

void tui_enter(void)
{
  tcgetattr(STDIN_FILENO, &original_termios);
  atexit(tui_exit);

  struct termios term;
  tcgetattr(STDIN_FILENO, &term);

  cfmakeraw(&term);
  term.c_cc[VMIN] = 0; // set minimum number of chars to 0 so a read() does not block

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);

  fputs(ALT_SCREEN_ON, stdout);
  fflush(stdout);
}
