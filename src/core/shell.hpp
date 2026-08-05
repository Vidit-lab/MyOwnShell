#pragma once

// Run the read-eval-print loop: report finished background jobs, print the
// prompt, read a line, route it to a pipeline or a single command, repeat.
//
// Does not return. The shell leaves through the `exit` builtin or Ctrl-D on an
// empty line, both of which call exit() directly.
[[noreturn]] void run_repl();
