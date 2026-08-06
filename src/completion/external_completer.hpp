#pragma once

#include <cstddef>
#include <string>
#include <vector>

// Run a `complete -C` script and read its suggestions from stdout, one per line.
//
// The script is invoked the way bash invokes one: argv is
// {script, command-name, current-word, previous-word}, with COMP_LINE and
// COMP_POINT exported. Blank lines are dropped, and a script that cannot be
// forked or exec'd simply yields no candidates.
std::vector<std::string> run_completer(const std::string &script, const std::string &cmd_name,
                                       const std::string &cur_word, const std::string &prev_word,
                                       const std::string &comp_line, std::size_t comp_point);
