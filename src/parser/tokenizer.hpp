#pragma once

#include <string>
#include <vector>

// Split a command line into words.
//
// Quoting follows POSIX:
//   - inside single quotes nothing is special, not even backslash;
//   - inside double quotes a backslash escapes only " \ $ ` and newline, and is
//     otherwise literal;
//   - outside quotes a backslash escapes any single character.
//
// Runs of unquoted spaces separate words; quoted segments that touch each other
// (or touch unquoted text) join into a single word, so 'foo'"bar" is one token.
std::vector<std::string> tokenize(const std::string &line);
