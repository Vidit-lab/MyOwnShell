#include "line/line_reader.hpp"

#include "completion/completion.hpp"
#include "line/terminal.hpp"

#include <cstdlib>
#include <iostream>

#include <unistd.h>

using namespace std;

namespace {

// Print the candidate list on its own line, then reprint the prompt and the
// line being edited so the user's cursor ends up back where it was.
void print_listing(const CompletionResult &completion, const string &prompt, const string &line) {
  cout << "\n";
  for (size_t i = 0; i < completion.listing.size(); ++i) {
    if (i > 0) cout << "  ";
    cout << completion.listing[i];
  }
  cout << "\n" << prompt << line << std::flush;
}

}  // namespace

string read_line_raw(const string &prompt) {
  RawMode raw;
  if (!raw.active()) {
    // Not a terminal: no editing and no completion, just read the line.
    string fallback_line;
    if (!getline(cin, fallback_line)) exit(0);
    return fallback_line;
  }

  string current_line = "";
  char ch;
  bool prev_tab = false;

  while (true) {
    ssize_t n = read(STDIN_FILENO, &ch, 1);

    if (n <= 0) {
      if (current_line.empty()) {
        raw.restore();      // exit() does not unwind locals
        cout << endl;
        exit(0);            // Ctrl-D on an empty line exits
      }
      break;                // partial line: run what we have
    }

    if (ch == '\n') {
      cout << endl;
      break;
    }
    else if (ch == '\t') {
      CompletionResult completion = complete_line(current_line, prev_tab);
      current_line = completion.line;

      if (completion.bell) cout << "\a" << std::flush;
      if (!completion.to_echo.empty()) cout << completion.to_echo << std::flush;
      if (completion.list) print_listing(completion, prompt, current_line);

      prev_tab = completion.arm_second_tab;
    }
    else if (ch == 127 || ch == 8) {
      if (!current_line.empty()) {
        current_line.pop_back();
        cout << "\b \b" << std::flush;
      }
      prev_tab = false;
    }
    else {
      current_line += ch;
      cout << ch << std::flush;
      prev_tab = false;
    }
  }

  return current_line;
}
