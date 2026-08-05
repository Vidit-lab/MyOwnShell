#pragma once

#include <string>
#include <vector>

// $PATH split into its directory components, in search order. Entries are
// returned verbatim — callers decide what to do with empty or unreadable ones.
std::vector<std::string> path_entries();

// Full path of the first executable named `name` on $PATH, or "" if there is
// none. "Executable" means access(X_OK) succeeds, matching what execv needs.
std::string find_executable(const std::string &name);
