#pragma once

#include <termios.h>

struct TerminalSize
{
  unsigned short rows;
  unsigned short cols;
};

class Tui
{
public:
  Tui() : original_termios_(get_current_termios()) { enter(); };
  ~Tui() { exit(); };

  Tui(const Tui &) = delete;
  Tui &operator=(const Tui &) = delete;
  Tui(Tui &&) = delete;
  Tui &operator=(Tui &&) = delete;

  void clear();
  TerminalSize get_size() const;

private:
  const struct termios original_termios_;

  void enter();
  void exit();
  static struct termios get_current_termios();
};
