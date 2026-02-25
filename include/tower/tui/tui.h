#ifndef TUI_H
#define TUI_H

#include <termios.h>

#include "tower/metric.h"

static struct termios original_terminal;

void tui_get_termios(struct termios *termios);

void tui_enter(void);

void tui_exit();

void tui_clear(void);

void draw_tui(const MemInfo *const meminfo);

#endif
