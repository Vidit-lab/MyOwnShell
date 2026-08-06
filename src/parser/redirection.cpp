#include "parser/redirection.hpp"

using namespace std;

RedirectionConfig parse_redirection(const vector<string> &tokens) {
  RedirectionConfig config;

  for (size_t i = 0; i < tokens.size(); ++i) {
    const string &token = tokens[i];
    bool is_stdout_trunc = (token == ">" || token == "1>");
    bool is_stderr_trunc = (token == "2>");
    bool is_stdout_append = (token == ">>" || token == "1>>");
    bool is_stderr_append = (token == "2>>");

    if (is_stdout_trunc || is_stderr_trunc || is_stdout_append || is_stderr_append) {
      if (i + 1 < tokens.size()) {
        config.active = true;
        config.file = tokens[i + 1];
        config.operator_idx = i;
        config.target_fd = (is_stderr_trunc || is_stderr_append) ? STDERR_FILENO : STDOUT_FILENO;
        config.open_flags |= (is_stdout_append || is_stderr_append) ? O_APPEND : O_TRUNC;
        break;
      }
    }
  }
  return config;
}
