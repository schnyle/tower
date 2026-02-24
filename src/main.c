#include <termios.h>
#include <unistd.h>

#include "tower/config.h"
#include "tower/log.h"
#include "tower/metric.h"

#define CLEAR_SCREEN "\033[2J"
#define CURSOR_HOME "\033[H"
#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"
#define ALT_SCREEN_ON "\033[?1049h"
#define ALT_SCREEN_OFF "\033[?1049l"

// Move cursor to row, col (1-indexed)
void move_cursor(int row, int col) { printf("\033[%d;%dH", row, col); }
static struct termios orig_termios;

void restore_terminal(void)
{
  // return to normal mode and leave alternate screen
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  fputs("\033[?1049l", stdout); // exit alternate screen
  fputs("\033[?25h", stdout);   // show cursor
  fflush(stdout);
}

void enter_raw_mode(void)
{
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(restore_terminal);

  struct termios raw = orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void draw_ui(const MemInfo *const meminfo)
{
  // Redraw from top each frame without flicker
  fputs(CURSOR_HOME, stdout);

  // Example: static layout, overwrite values in place
  printf("System Monitor\r\n");
  printf("==============\r\n\r\n");

  // You'd pull real values here (read /proc/stat, /proc/meminfo, etc.)
  printf("MemInfo:\r\n");
  printf("\tTotal: %ld kB\r\n", meminfo->total_kb);
  printf("\tFree: %ld kB\r\n", meminfo->free_kb);
  printf("\tAvailable: %ld kB\r\n", meminfo->available_kb);
  printf("\tSwap Total: %ld kB\r\n", meminfo->swap_total_kb);
  printf("\tSwap Free: %ld kB\r\n", meminfo->swap_free_kb);
  printf("\n");
  printf("\r\nPress 'q' to quit\r\n");

  fflush(stdout);
}

int main()
{
  init_config();
  LOG_INFO("configuration and logger initialized");

  MemInfo meminfo;

  enter_raw_mode();
  fputs(CURSOR_HIDE, stdout);
  fputs(CLEAR_SCREEN, stdout);

  while (1)
  {
    get_proc_meminfo(&meminfo);

    draw_ui(&meminfo);

    char c;
    if (read(STDIN_FILENO, &c, 1) == 1 && c == 'q')
      break;

    sleep(1);
  }

  return 0;
}
