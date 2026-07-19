#pragma once

struct TerminalSize
{
  unsigned short rows;
  unsigned short cols;
};

void tui_enter(void);

void tui_clear(void);

TerminalSize tui_get_size();
