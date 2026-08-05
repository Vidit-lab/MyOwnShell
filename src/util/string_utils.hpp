#pragma once

#include <string>
#include <vector>

// Longest prefix shared by every string in `values`; "" for an empty list.
// Used by tab completion to decide how much of a word it can safely fill in.
std::string longest_common_prefix(const std::vector<std::string> &values);

// Concatenate `parts` with `separator` between them.
std::string join(const std::vector<std::string> &parts, const std::string &separator);
