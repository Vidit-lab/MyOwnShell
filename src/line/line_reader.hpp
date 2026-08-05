#pragma once

#include <string>

// Read one line from the terminal, handling Tab completion, backspace and
// Ctrl-D. This is the shell's line editor; it is where key handling belongs,
// and where history navigation will hook in.
//
// `prompt` is needed only to redraw the line after a completion listing has
// scrolled it, so it must match what the caller already printed.
//
// When stdin is not a terminal there is nothing to edit and the line is read
// with getline instead. Either way, EOF (Ctrl-D) on an empty line exits the
// shell rather than returning.
std::string read_line_raw(const std::string &prompt);
