#pragma once

#include <termios.h>

// Puts the terminal into raw mode — no canonical line buffering, no echo, so
// the shell sees each keystroke as it is typed — and restores the previous
// settings when it goes out of scope.
//
// When stdin is not a terminal (a pipe or a file), construction is a no-op:
// active() returns false and nothing is changed. Callers fall back to plain
// line-buffered reads.
class RawMode {
 public:
  RawMode();
  ~RawMode();

  RawMode(const RawMode &) = delete;
  RawMode &operator=(const RawMode &) = delete;

  bool active() const { return active_; }

  // Restore the saved settings now rather than at scope exit. Needed on paths
  // that leave via exit(), which does not unwind local objects. Idempotent.
  void restore();

 private:
  struct termios original_ {};
  bool active_ = false;
};
