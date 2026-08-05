#pragma once

#include <string>
#include <vector>

// Candidates for the first word of a line: builtins plus every executable on
// $PATH starting with `prefix`. De-duplicated and sorted. Empty for an empty
// prefix — Tab on a bare prompt should not list the whole of $PATH.
std::vector<std::string> get_command_completions(const std::string &prefix);

// Candidates for a filename argument. `word` may carry a directory part
// ("src/ma"), which is preserved on every match. Hidden entries are offered
// only when the leaf being completed itself starts with '.'.
std::vector<std::string> get_file_completions(const std::string &word);

// What a Tab press should do to the line being edited.
//
// complete_line() decides; the line reader is left to perform the terminal I/O.
// That split keeps the key loop free of completion logic and makes the
// completion rules testable without a tty.
struct CompletionResult {
  std::string line;     // the line after completion (unchanged when nothing was inserted)
  std::string to_echo;  // text to write at the cursor; "" when there is none
  bool bell = false;    // ring the terminal bell
  bool list = false;    // print `listing`, then redraw prompt + line
  std::vector<std::string> listing;  // display names, directories already marked '/'

  // New value for the caller's "previous key was Tab" flag. Only an ambiguous
  // prefix that inserted nothing sets this, so that the *next* Tab lists the
  // candidates — bash's bell-then-list behaviour.
  bool arm_second_tab = false;
};

// Resolve a Tab press on `line`. `second_tab` says whether the previous key was
// also a Tab.
//
// The word under the cursor is the last space-separated run: if it is the first
// word the command completer runs, otherwise a `complete -C` script if one is
// registered for the command, and failing that the filesystem completer.
CompletionResult complete_line(const std::string &line, bool second_tab);
