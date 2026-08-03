#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <set>
#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;
namespace fs = std::filesystem;

// ==========================================
// 0. Global Registry Configuration
// ==========================================
const vector<string> builtinCommands = {"echo", "exit", "pwd", "cd", "type"};
const unordered_set<string> listCommands(builtinCommands.begin(), builtinCommands.end());

struct RedirectionConfig {
  bool active = false;
  string file = "";
  int target_fd = STDOUT_FILENO;
  int open_flags = O_WRONLY | O_CREAT;
  size_t operator_idx = 0;
};

// ==========================================
// 1. Core Built-in Handlers
// ==========================================
void echoCommand(const vector<string> &p) {
  for (size_t i = 1; i < p.size(); i++) {
    cout << p[i];
    if (i < p.size() - 1) cout << " ";
  }
  cout << endl;
}

void pwdCommand() {
  cout << fs::current_path().string() << endl;
}

void typeCommand(const vector<string> &splitwords) {
  if (splitwords.size() < 2) return;
  string cmd = splitwords[1];
  if (listCommands.count(cmd)) {
    cout << cmd << " is a shell builtin\n";
    return;
  }

  const char* env_path = getenv("PATH");
  string pathvar = env_path ? env_path : "";
  istringstream path_stream(pathvar);
  string pathsplit;
  while (getline(path_stream, pathsplit, ':')) {
    string filepath = pathsplit + '/' + cmd;
    if (access(filepath.c_str(), X_OK) == 0) {
      cout << cmd << " is " << filepath << endl;
      return;
    }
  }
  cout << cmd << ": not found\n";
}

void externalProgram(const vector<string> &splitwords) {
  string cmd = splitwords[0];
  const char* env_path = getenv("PATH");
  string pathvar = env_path ? env_path : "";
  istringstream path_stream(pathvar);
  string pathsplit;
  while (getline(path_stream, pathsplit, ':')) {
    string filepath = pathsplit + '/' + cmd;
    if (access(filepath.c_str(), X_OK) == 0) {
      vector<char *> argv;
      for (auto &a : splitwords) {
        argv.push_back(const_cast<char*>(a.c_str()));
      }
      argv.push_back(nullptr);

      execv(filepath.c_str(), argv.data());
      perror("execv failed");
      exit(1);
    }
  }
  cerr << cmd << ": not found\n";
  exit(1);
}

// ==========================================
// 2. State-Machine Argument Parser
// ==========================================
void splitwords(const string &command, vector<string> &splitted) {
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
}

// ==========================================
// 3. Dynamic Executable Completion Engine
// ==========================================
vector<string> get_all_completions(const string &prefix) {
  if (prefix.empty()) return {};

  set<string> unique_matches;

  for (const auto& target : builtinCommands) {
    if (target.rfind(prefix, 0) == 0) {
      unique_matches.insert(target);
    }
  }

  const char* env_path = getenv("PATH");
  string pathvar = env_path ? env_path : "";
  istringstream path_stream(pathvar);
  string pathsplit;

  while (getline(path_stream, pathsplit, ':')) {
    if (pathsplit.empty() || !fs::exists(pathsplit) || !fs::is_directory(pathsplit)) continue;

    try {
      for (const auto& entry : fs::directory_iterator(pathsplit)) {
        try {
          if (fs::is_directory(entry.path())) continue;

          string filename = entry.path().filename().string();
          if (filename.rfind(prefix, 0) == 0 && access(entry.path().string().c_str(), X_OK) == 0) {
            unique_matches.insert(filename);
          }
        } catch (...) {
          continue; // skip a single bad entry (e.g. broken symlink), keep scanning
        }
      }
    } catch (...) {
      continue; // skip an unreadable directory entirely
    }
  }

  return vector<string>(unique_matches.begin(), unique_matches.end());
}

// Longest common prefix across all matches: lets Tab extend as far as it can
// unambiguously before it has to give up and ring the bell.
string longest_common_prefix(const vector<string>& v) {
  if (v.empty()) return "";
  string p = v[0];
  for (size_t i = 1; i < v.size(); ++i) {
    size_t j = 0;
    while (j < p.size() && j < v[i].size() && p[j] == v[i][j]) ++j;
    p.resize(j);
    if (p.empty()) break;
  }
  return p;
}

// ==========================================
// 4. Isolated Live Input Driver
// ==========================================
string read_line_raw() {
  struct termios original_terminal;
  if (tcgetattr(STDIN_FILENO, &original_terminal) < 0) {
    string fallback_line;
    if (!getline(cin, fallback_line)) exit(0);
    return fallback_line;
  }

  struct termios raw_terminal = original_terminal;
  raw_terminal.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &raw_terminal);

  string current_line = "";
  char ch;
  bool prev_tab = false;   // was the previous keypress also a Tab?

  while (true) {
    ssize_t n = read(STDIN_FILENO, &ch, 1);

    if (n <= 0) {
      // EOF (Ctrl-D) or read error.
      if (current_line.empty()) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
        cout << endl;
        exit(0);            // bash-like: Ctrl-D on an empty line exits the shell
      }
      break;                // partial line typed: execute what we have
    }

    if (ch == '\n') {
      cout << endl;
      break;
    }
    else if (ch == '\t') {
      // Command completion only for now. Once there's a space we're completing
      // an argument (file/path completion) which isn't implemented yet.
      if (current_line.find(' ') != string::npos) {
        cout << "\a" << std::flush;
      }  else {
        // matches comes from a set<string>, so it's already alphabetically sorted.
        vector<string> matches = get_all_completions(current_line);
 
        if (matches.empty()) {
          cout << "\a" << std::flush;
          prev_tab = false;
        } else if (matches.size() == 1) {
          string completion = matches[0].substr(current_line.length()) + " ";
          current_line = matches[0] + " ";
          cout << completion << std::flush;
          prev_tab = false;
        } else {
          string lcp = longest_common_prefix(matches);
          if (lcp.size() > current_line.size()) {
            string completion = lcp.substr(current_line.length());
            current_line = lcp;
            cout << completion << std::flush;
            prev_tab = false;
          } else if (!prev_tab) {
            cout << "\a" << std::flush;
            prev_tab = true;
          } else {
            cout << "\n";
            for (size_t i = 0; i < matches.size(); ++i) {
              if (i > 0) cout << "  ";
              cout << matches[i];
            }
            cout << "\n$ " << current_line << std::flush;
            prev_tab = false;
          }
        }
      }
    }

    else if (ch == 127 || ch == 8) {
      if (!current_line.empty()) {
        current_line.pop_back();
        cout << "\b \b" << std::flush;
      }
    }
    else {
      current_line += ch;
      cout << ch << std::flush;
    }
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
  return current_line;
}

// ==========================================
// 5. Redirection Parser Helper
// ==========================================
RedirectionConfig parse_redirection(const vector<string> &splitwords) {
  RedirectionConfig config;

  for (size_t i = 0; i < splitwords.size(); ++i) {
    const string &token = splitwords[i];
    bool is_stdout_trunc  = (token == ">" || token == "1>");
    bool is_stderr_trunc  = (token == "2>");
    bool is_stdout_append = (token == ">>" || token == "1>>");
    bool is_stderr_append = (token == "2>>");

    if (is_stdout_trunc || is_stderr_trunc || is_stdout_append || is_stderr_append) {
      if (i + 1 < splitwords.size()) {
        config.active = true;
        config.file = splitwords[i + 1];
        config.operator_idx = i;
        config.target_fd = (is_stderr_trunc || is_stderr_append) ? STDERR_FILENO : STDOUT_FILENO;
        config.open_flags |= (is_stdout_append || is_stderr_append) ? O_APPEND : O_TRUNC;
        break;
      }
    }
  }
  return config;
}

// ==========================================
// 6. Command Routing and Execution Engine
// ==========================================

void executeCommand(const vector<string> &splitwords) {
  if (splitwords.empty()) return;
  string cmd = splitwords[0];

  if (cmd == "exit") {
    exit(0);
  }
  if (cmd == "cd") {
    if (splitwords.size() < 2) return;
    string target = splitwords[1];
    if (target == "~") {
      const char* home = getenv("HOME");
      target = home ? home : "";
    }
    if (fs::exists(target) && fs::is_directory(target)) {
      fs::current_path(target);
    } else {
      cout << "cd: " << splitwords[1] << ": No such file or directory\n";
    }
    return;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
    return;
  }

  if (pid == 0) {
    RedirectionConfig redir = parse_redirection(splitwords);

    vector<string> active_args = splitwords;
    if (redir.active) {
      active_args.resize(redir.operator_idx);
    }

    if (active_args.empty()) exit(0);

    if (redir.active) {
      int file_fd = open(redir.file.c_str(), redir.open_flags, 0644);
      if (file_fd < 0) {
        perror("open failed");
        exit(1);
      }
      dup2(file_fd, redir.target_fd);
      close(file_fd);
    }

    string child_cmd = active_args[0];
    if (child_cmd == "echo") {
      echoCommand(active_args);
    } else if (child_cmd == "type") {
      typeCommand(active_args);
    } else if (child_cmd == "pwd") {
      pwdCommand();
    } else {
      externalProgram(active_args);
    }
    exit(0);
  }
  else {
    int status;
    waitpid(pid, &status, 0);
  }
}

// ==========================================
// 7. Main Program Loop
// ==========================================

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    cout << "$ " << std::flush;

    string command_line = read_line_raw();  // exits directly on Ctrl-D at empty line

    vector<string> splitcommand;
    splitwords(command_line, splitcommand);
    executeCommand(splitcommand);
  }
  return 0;
}