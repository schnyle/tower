#ifndef TUI_H
#define TUI_H

#include <termios.h>

#include "tower/metric.h"

void front_set(int, int, const char *);

void tui_get_termios(struct termios *);

void tui_enter(void);

void tui_exit();

void tui_clear(void);

void draw_tui(const MemInfo *const);

#endif
