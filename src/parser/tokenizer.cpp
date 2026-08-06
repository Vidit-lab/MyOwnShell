#include "parser/tokenizer.hpp"

using namespace std;

vector<string> tokenize(const string &command) {
  vector<string> splitted;

  string current_arg = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  bool inside_word = false;

  for (size_t i = 0; i < command.length(); ++i) {
    char c = command[i];

    if (in_single_quotes) {
      if (c == '\'') in_single_quotes = false;
      else current_arg += c;
    } else if (in_double_quotes) {
      if (c == '\\') {
        if (i + 1 < command.length()) {
          char next_c = command[i + 1];
          if (next_c == '"' || next_c == '\\' || next_c == '$' || next_c == '`') {
            i++;
            current_arg += next_c;
          } else if (next_c == '\n') {
            i++;
          } else {
            current_arg += c;
          }
        } else {
          current_arg += c;
        }
      } else if (c == '"') {
        in_double_quotes = false;
      } else {
        current_arg += c;
      }
    } else {
      if (c == '\\') {
        if (i + 1 < command.length()) {
          i++;
          current_arg += command[i];
          inside_word = true;
        }
      } else if (c == '\'') {
        in_single_quotes = true;
        inside_word = true;
      } else if (c == '"') {
        in_double_quotes = true;
        inside_word = true;
      } else if (c == ' ') {
        if (inside_word) {
          splitted.push_back(current_arg);
          current_arg = "";
          inside_word = false;
        }
      } else {
        current_arg += c;
        inside_word = true;
      }
    }
  }
  if (inside_word) splitted.push_back(current_arg);

  return splitted;
}
