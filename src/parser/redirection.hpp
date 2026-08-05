#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

// Where one command's output should be sent, as parsed from its token list.
struct RedirectionConfig {
  bool active = false;
  std::string file = "";
  int target_fd = STDOUT_FILENO;
  int open_flags = O_WRONLY | O_CREAT;
  std::size_t operator_idx = 0;  // index of the operator token; the command's
                                 // own arguments end just before it
};

// Find the first redirection operator in `tokens`, recognising >, >>, 1>, 1>>,
// 2> and 2>>. An operator with no filename after it is ignored. Returns an
// inactive config when the command has no redirection.
RedirectionConfig parse_redirection(const std::vector<std::string> &tokens);
