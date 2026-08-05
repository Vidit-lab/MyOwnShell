#pragma once

#include <string>
#include <vector>

// Replace the calling process with the program named by args[0], resolved on
// $PATH. Only ever called from a forked child — it never returns: it execs, or
// reports "<cmd>: not found" on stderr and exits 1.
[[noreturn]] void externalProgram(const std::vector<std::string> &args);

// Run one command in a process that is already set up for it: a pipeline stage
// with its pipe fds wired up, or the child of a simple command with its
// redirections applied. Builtins with a child handler run here; everything else
// is exec'd. Never returns.
[[noreturn]] void runInProcess(const std::vector<std::string> &args);
