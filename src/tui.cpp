#include "tui.hpp"

#include <cstdio>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define ALT_SCREEN_ON "\033[?1049h"
#define ALT_SCREEN_OFF "\033[?1049l"

void Tui::clear(void)
{
  fputs(CURSOR_HOME, stdout);
  fputs(CURSOR_HIDE, stdout);
  fputs(CLEAR_SCREEN, stdout);
  fflush(stdout);
}

TerminalSize Tui::get_size() const
{
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_row == 0 || ws.ws_col == 0)
  {
    return {24, 80};
  }
  return {ws.ws_row, ws.ws_col};
}

void Tui::enter(void)
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);

  cfmakeraw(&t);
  t.c_cc[VMIN] = 0; // set minimum number of chars to 0 so a read() does not block

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &t);

  fputs(ALT_SCREEN_ON, stdout);
  clear();
}

void Tui::exit(void)
{
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios_);
  fputs(ALT_SCREEN_OFF, stdout);
  fputs(CURSOR_SHOW, stdout);
  fflush(stdout);
}

struct termios Tui::get_current_termios()
{
  struct termios t;
  tcgetattr(STDIN_FILENO, &t);
  return t;
}
