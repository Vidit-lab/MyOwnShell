#include "core/shell.hpp"

#include <iostream>

int main() {
  // Unbuffered output: the shell interleaves its own writes with those of the
  // children it forks, and a buffered stream would let the two arrive out of
  // order — or be duplicated, since a forked child inherits a copy of whatever
  // is still sitting in the buffer.
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  run_repl();
}
