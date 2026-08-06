#include "completion/completion.hpp"

#include "builtins/builtins.hpp"
#include "completion/completion_specs.hpp"
#include "completion/external_completer.hpp"
#include "parser/tokenizer.hpp"
#include "util/path.hpp"
#include "util/string_utils.hpp"

#include <algorithm>
#include <filesystem>
#include <set>

#include <unistd.h>

using namespace std;
namespace fs = std::filesystem;

// First word: builtins + executables on PATH.
vector<string> get_command_completions(const string &prefix) {
  if (prefix.empty()) return {};

  set<string> unique_matches;

  for (const auto &target : builtin_names()) {
    if (target.rfind(prefix, 0) == 0) {
      unique_matches.insert(target);
    }
  }

  for (const string &pathsplit : path_entries()) {
    if (pathsplit.empty() || !fs::exists(pathsplit) || !fs::is_directory(pathsplit)) continue;

    try {
      for (const auto &entry : fs::directory_iterator(pathsplit)) {
        try {
          if (fs::is_directory(entry.path())) continue;

          string filename = entry.path().filename().string();
          if (filename.rfind(prefix, 0) == 0 && access(entry.path().string().c_str(), X_OK) == 0) {
            unique_matches.insert(filename);
          }
        } catch (...) {
          continue;
        }
      }
    } catch (...) {
      continue;
    }
  }

  return vector<string>(unique_matches.begin(), unique_matches.end());
}

vector<string> get_file_completions(const string &word) {
  size_t slash = word.find_last_of('/');
  string dir_part = (slash == string::npos) ? "" : word.substr(0, slash + 1);
  string leaf     = (slash == string::npos) ? word : word.substr(slash + 1);

  string scan_dir = dir_part.empty() ? "." : dir_part;

  vector<string> matches;
  error_code ec;
  if (!fs::is_directory(scan_dir, ec)) return matches;

  bool want_hidden = !leaf.empty() && leaf[0] == '.';

  for (const auto &entry : fs::directory_iterator(scan_dir, ec)) {
    string name = entry.path().filename().string();
    if (!want_hidden && !name.empty() && name[0] == '.') continue;
    if (name.rfind(leaf, 0) == 0) {
      matches.push_back(dir_part + name);
    }
  }

  sort(matches.begin(), matches.end());
  return matches;
}

CompletionResult complete_line(const string &line, bool second_tab) {
  CompletionResult result;
  result.line = line;

  // Complete the last word: first word => command, otherwise => filename.
  size_t sp = line.find_last_of(' ');
  bool completing_command = (sp == string::npos);
  string word      = completing_command ? line : line.substr(sp + 1);
  string line_head = completing_command ? ""   : line.substr(0, sp + 1);

  vector<string> matches;
  bool from_filesystem = false;
  if (completing_command) {
    matches = get_command_completions(word);
  } else {
    vector<string> head_tokens = tokenize(line_head);
    string command_name = head_tokens.empty() ? "" : head_tokens.front();
    string prev_word    = head_tokens.empty() ? "" : head_tokens.back();
    string spec = lookup_completion_spec(command_name);
    if (!spec.empty()) {
      matches = run_completer(spec, command_name, word, prev_word, line, line.length());
    } else {
      matches = get_file_completions(word);
      from_filesystem = true;
    }
  }

  if (matches.empty()) {
    result.bell = true;
    return result;
  }

  if (matches.size() == 1) {
    const string &sole = matches[0];
    bool is_dir = from_filesystem && fs::is_directory(sole);
    string suffix = is_dir ? "/" : " ";  // dir keeps you moving, file closes the token

    result.to_echo = sole.substr(word.length()) + suffix;
    result.line = line_head + sole + suffix;
    return result;
  }

  string lcp = longest_common_prefix(matches);
  if (lcp.size() > word.size()) {
    result.to_echo = lcp.substr(word.length());
    result.line = line_head + lcp;
    return result;
  }

  if (!second_tab) {
    result.bell = true;  // first Tab: bell
    result.arm_second_tab = true;
    return result;
  }

  // second Tab: list basenames alphabetically, dirs marked with '/'
  result.list = true;
  for (const string &m : matches) {
    string base = m.substr(m.find_last_of('/') + 1);
    if (from_filesystem && fs::is_directory(m)) base += "/";
    result.listing.push_back(base);
  }
  return result;
}
