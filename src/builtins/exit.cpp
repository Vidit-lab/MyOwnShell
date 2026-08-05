#include "builtins/builtins.hpp"

#include <cstdlib>

#include <unistd.h>

using namespace std;

// In the shell process, `exit` ends the shell.
void builtin_exit(const vector<string> &) {
  std::exit(0);
}

// In a forked stage, `exit` must end that stage only. _exit rather than exit:
// the child shares the parent's stdio buffers and atexit handlers, and must not
// run either on its way out.
void builtin_exit_child(const vector<string> &) {
  _exit(0);
}
