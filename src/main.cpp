#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <set>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

using namespace std;
namespace fs = std::filesystem;

// Single source of truth for builtins; type + completion derive from this.
const vector<string> builtinCommands = {"echo", "exit", "pwd", "cd", "type", "complete", "jobs"};
const unordered_set<string> listCommands(builtinCommands.begin(), builtinCommands.end());

// command -> completer script path, registered via `complete -C`.
map<string, string> completionSpecs;

struct BackgroundJob {
  pid_t pid;
  string command;
};
map<int, BackgroundJob> jobTable;

struct RedirectionConfig {
  bool active = false;
  string file = "";
  int target_fd = STDOUT_FILENO;
  int open_flags = O_WRONLY | O_CREAT;
  size_t operator_idx = 0;
};

// ---- Built-in handlers ----

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

void runInProcess(const vector<string> &args) {
  if (args.empty()) _exit(0);
  const string &cmd = args[0];
  if (cmd == "echo")      echoCommand(args);
  else if (cmd == "type") typeCommand(args);
  else if (cmd == "pwd")  pwdCommand();
  else if (cmd == "exit") _exit(0);              // subshell: exit ends this stage only
  else                    externalProgram(args); // execs, or exits on not-found
  _exit(0);
}

// ---- Argument parser (quote/escape aware) ----

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

// ---- Completion engines ----

// First word: builtins + executables on PATH.
vector<string> get_command_completions(const string &prefix) {
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

  for (const auto& entry : fs::directory_iterator(scan_dir, ec)) {
    string name = entry.path().filename().string();
    if (!want_hidden && !name.empty() && name[0] == '.') continue;
    if (name.rfind(leaf, 0) == 0) {
      matches.push_back(dir_part + name);
    }
  }

  sort(matches.begin(), matches.end());
  return matches;
}

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

vector<string> run_completer(const string& script, const string& cmd_name,
                             const string& cur_word, const string& prev_word,
                             const string& comp_line, size_t comp_point) {
  int fds[2];
  if (pipe(fds) < 0) return {};

  pid_t pid = fork();
  if (pid < 0) { close(fds[0]); close(fds[1]); return {}; }

  if (pid == 0) {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    close(fds[1]);
    setenv("COMP_LINE", comp_line.c_str(), 1);
    setenv("COMP_POINT", to_string(comp_point).c_str(), 1);
    vector<char*> argv = {
      const_cast<char*>(script.c_str()),
      const_cast<char*>(cmd_name.c_str()),
      const_cast<char*>(cur_word.c_str()),
      const_cast<char*>(prev_word.c_str()),
      nullptr
    };
    execv(script.c_str(), argv.data());
    _exit(127);
  }

  close(fds[1]);
  string out;
  char buf[4096];
  ssize_t n;
  while ((n = read(fds[0], buf, sizeof(buf))) > 0) out.append(buf, n);
  close(fds[0]);
  waitpid(pid, nullptr, 0);

  vector<string> lines;
  istringstream iss(out);
  string line;
  while (getline(iss, line)) {
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}

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
  bool prev_tab = false;

  while (true) {
    ssize_t n = read(STDIN_FILENO, &ch, 1);

    if (n <= 0) {
      if (current_line.empty()) {
        tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
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
      // Complete the last word: first word => command, otherwise => filename.
      size_t sp = current_line.find_last_of(' ');
      bool completing_command = (sp == string::npos);
      string word      = completing_command ? current_line : current_line.substr(sp + 1);
      string line_head = completing_command ? ""           : current_line.substr(0, sp + 1);

      vector<string> matches;
      bool from_filesystem = false;
      if (completing_command) {
        matches = get_command_completions(word);
      } else {
        vector<string> head_tokens;
        splitwords(line_head, head_tokens);
        string command_name = head_tokens.empty() ? "" : head_tokens.front();
        string prev_word    = head_tokens.empty() ? "" : head_tokens.back();
        auto it = completionSpecs.find(command_name);
        if (it != completionSpecs.end()) {
          matches = run_completer(it->second, command_name, word, prev_word,
                                  current_line, current_line.length());
        } else {
          matches = get_file_completions(word);
          from_filesystem = true;
        }
      }

      if (matches.empty()) {
        cout << "\a" << std::flush;
        prev_tab = false;
      } else if (matches.size() == 1) {
        const string& sole = matches[0];
        bool is_dir = from_filesystem && fs::is_directory(sole);
        string suffix = is_dir ? "/" : " ";   // dir keeps you moving, file closes the token

        string appended = sole.substr(word.length()) + suffix;
        current_line = line_head + sole + suffix;
        cout << appended << std::flush;
        prev_tab = false;
      } else {
        string lcp = longest_common_prefix(matches);
        if (lcp.size() > word.size()) {
          string appended = lcp.substr(word.length());
          current_line = line_head + lcp;
          cout << appended << std::flush;
          prev_tab = false;
        } else if (!prev_tab) {
          cout << "\a" << std::flush;   // first Tab: bell
          prev_tab = true;
        } else {
          // second Tab: list basenames alphabetically, dirs marked with '/'
          cout << "\n";
          for (size_t i = 0; i < matches.size(); ++i) {
            if (i > 0) cout << "  ";
            const string& m = matches[i];
            string base = m.substr(m.find_last_of('/') + 1);
            if (from_filesystem && fs::is_directory(m)) base += "/";
            cout << base;
          }
          cout << "\n$ " << current_line << std::flush;
          prev_tab = false;
        }
      }
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

  tcsetattr(STDIN_FILENO, TCSANOW, &original_terminal);
  return current_line;
}

// ---- Redirection parsing ----

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

// ---- Background job reaping ----

// The two most-recently-started job ids (0 if absent), for the +/- markers.
pair<int, int> current_and_previous_ids() {
  int max_id = jobTable.empty() ? 0 : jobTable.rbegin()->first;
  int second_id = 0;
  if (jobTable.size() >= 2) {
    auto it = jobTable.rbegin();
    ++it;
    second_id = it->first;
  }
  return {max_id, second_id};
}

char job_marker(int id, int max_id, int second_id) {
  if (id == max_id) return '+';
  if (id == second_id) return '-';
  return ' ';
}

// One listing line. `running` picks the status text AND whether '&' is shown.
void print_job_line(int id, char marker, const string& command, bool running) {
  string field = running ? "Running" : "Done";
  if (field.size() < 24) field.append(24 - field.size(), ' ');
  cout << "[" << id << "]" << marker << "  " << field << command;
  if (running) cout << " &";
  cout << "\n";
}

set<int> poll_exited_jobs() {
  set<int> exited;
  for (const auto& [id, job] : jobTable) {
    int status;
    pid_t r = waitpid(job.pid, &status, WNOHANG);
    if (r == job.pid && WIFEXITED(status)) exited.insert(id);
  }
  return exited;
}

void reap_jobs(bool also_list_running) {
  set<int> exited = poll_exited_jobs();
  if (exited.empty() && !also_list_running) return;

  auto [max_id, second_id] = current_and_previous_ids();
  for (const auto& [id, job] : jobTable) {
    bool running = (exited.count(id) == 0);
    if (running && !also_list_running) continue;   // sweep: skip still-running
    print_job_line(id, job_marker(id, max_id, second_id), job.command, running);
  }

  for (int id : exited) jobTable.erase(id);
}

// ---- Command routing ----

void executeCommand(const vector<string> &args) {
  if (args.empty()) return;

  bool background = (args.back() == "&");
  vector<string> splitwords(args.begin(), args.end() - (background ? 1 : 0));
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

  // Parent-side: complete mutates shell state that must outlive one command.
  if (cmd == "complete") {
    if (splitwords.size() >= 3 && splitwords[1] == "-p") {
      const string& name = splitwords[2];
      auto it = completionSpecs.find(name);
      if (it != completionSpecs.end()) {
        cout << "complete -C '" << it->second << "' " << name << "\n";
      } else {
        cout << "complete: " << name << ": no completion specification\n";
      }
    } else if (splitwords.size() >= 4 && splitwords[1] == "-C") {
      completionSpecs[splitwords[3]] = splitwords[2];
    } else if (splitwords.size() >= 3 && splitwords[1] == "-r") {
      completionSpecs.erase(splitwords[2]);
    }
    return;
  }

  // Parent-side: jobs reaps finished jobs and lists all in job-number order.
  if (cmd == "jobs") {
    reap_jobs(/*also_list_running=*/true);
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

    runInProcess(active_args);
  }
  else {
    if (background) {
      string joined;
      for (size_t i = 0; i < splitwords.size(); ++i) {
        if (i) joined += " ";
        joined += splitwords[i];
      }
      // Recycle numbers: reuse freed slots instead of growing forever.
      int job_id = jobTable.empty() ? 1 : jobTable.rbegin()->first + 1;
      jobTable[job_id] = {pid, joined};
      cout << "[" << job_id << "] " << pid << "\n";
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

// ---- Pipelines ----

// Split a token list on '|' into its command stages: ["cat","f","|","wc"] -> {["cat","f"], ["wc"]}.
vector<vector<string>> split_on_pipe(const vector<string> &tokens) {
  vector<vector<string>> stages;
  vector<string> cur;
  for (const auto &t : tokens) {
    if (t == "|") { stages.push_back(cur); cur.clear(); }
    else cur.push_back(t);
  }
  stages.push_back(cur);
  return stages;
}

void runPipeline(const vector<string> &tokens) {
  vector<vector<string>> stages = split_on_pipe(tokens);
  int n = (int)stages.size();

  int prev_read = -1;                 // read end feeding the current stage's stdin
  vector<pid_t> pids;

  for (int i = 0; i < n; ++i) {
    bool has_next = (i < n - 1);
    int fds[2] = {-1, -1};
    if (has_next && pipe(fds) < 0) { perror("pipe failed"); break; }

    pid_t pid = fork();
    if (pid == 0) {
      if (prev_read != -1) dup2(prev_read, STDIN_FILENO);   // stdin from previous stage
      if (has_next)        dup2(fds[1], STDOUT_FILENO);      // stdout to next stage
      if (prev_read != -1) close(prev_read);
      if (has_next) { close(fds[0]); close(fds[1]); }
      runInProcess(stages[i]);        // builtin in-process, else exec
      _exit(1);
    }

    pids.push_back(pid);
    if (prev_read != -1) close(prev_read);   // parent is done with the incoming read end
    if (has_next) {
      close(fds[1]);                          // parent never writes
      prev_read = fds[0];                      // hand the read end to the next stage
    }
  }

  for (pid_t p : pids) waitpid(p, nullptr, 0);
}

// ---- Main loop ----

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    reap_jobs(/*also_list_running=*/false);   // report+clear finished jobs first
    cout << "$ " << std::flush;

    string command_line = read_line_raw();

    vector<string> splitcommand;
    splitwords(command_line, splitcommand);

    if (find(splitcommand.begin(), splitcommand.end(), "|") != splitcommand.end())
      runPipeline(splitcommand);
    else
      executeCommand(splitcommand);
  }
  return 0;
}