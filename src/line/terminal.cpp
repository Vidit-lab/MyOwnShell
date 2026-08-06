#include "line/terminal.hpp"

#include <unistd.h>

RawMode::RawMode() {
  if (tcgetattr(STDIN_FILENO, &original_) < 0) return;  // not a terminal

  struct termios raw_terminal = original_;
  raw_terminal.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw_terminal);
  active_ = true;
}

void RawMode::restore() {
  if (!active_) return;
  tcsetattr(STDIN_FILENO, TCSANOW, &original_);
  active_ = false;
}

RawMode::~RawMode() {
  restore();
}
