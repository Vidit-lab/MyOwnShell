#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;
const unordered_set<string> listCommands = {"type", "echo", "exit", "pwd", "cd"};

// ==========================================
// 1. Core Core Built-in Handlers
// ==========================================

void echoCommand(const vector<string> &p) {
  for (size_t i = 1; i < p.size(); i++) {
    cout << p[i];
    if (i < p.size() - 1) cout << " ";
  }
  cout << endl;
}

void pwdCommand() {
  cout << std::filesystem::current_path().string() << endl;
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
  cout << cmd << ": not found\n";
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
// 3. Command Routing and Execution Engine
// ==========================================

void executeCommand(const vector<string> &splitwords) {
  if (splitwords.empty()) return;
  string cmd = splitwords[0];

  // 1. Handle state-modifying built-ins immediately in the parent process
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
    if (std::filesystem::exists(target) && std::filesystem::is_directory(target)) {
      std::filesystem::current_path(target);
    } else {
      cout << "cd: " << splitwords[1] << ": No such file or directory\n";
    }
    return;
  }

  // 2. Fork isolated child for all other execution profiles
  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
    return;
  }

  if (pid == 0) { // CHILD PROCESS INNER LOGIC
    string redirect_file = "";
    bool do_redirect = false;
    int target_fd = STDOUT_FILENO;
    int open_flags = O_WRONLY | O_CREAT; // Base file creation flags
    size_t redirect_idx = 0;

    // Scan arguments inside the isolated process space
    for (size_t i = 0; i < splitwords.size(); ++i) {
      if (splitwords[i] == ">" || splitwords[i] == "1>") {
        if (i + 1 < splitwords.size()) {
          redirect_file = splitwords[i + 1];
          do_redirect = true;
          target_fd = STDOUT_FILENO;
          open_flags |= O_TRUNC; 
          redirect_idx = i;
          break;
        }
      } else if (splitwords[i] == "2>") {
        if (i + 1 < splitwords.size()) {
          redirect_file = splitwords[i + 1];
          do_redirect = true;
          target_fd = STDERR_FILENO;
          open_flags |= O_TRUNC; 
          redirect_idx = i;
          break;
        }
      } else if (splitwords[i] == ">>" || splitwords[i] == "1>>") { // Capture stdout append
        if (i + 1 < splitwords.size()) {
          redirect_file = splitwords[i + 1];
          do_redirect = true;
          target_fd = STDOUT_FILENO;
          open_flags |= O_APPEND; 
          redirect_idx = i;
          break;
        }
      } else if (splitwords[i] == "2>>") { // Capture stderr append
        if (i + 1 < splitwords.size()) {
          redirect_file = splitwords[i + 1];
          do_redirect = true;
          target_fd = STDERR_FILENO; 
          open_flags |= O_APPEND;    
          redirect_idx = i;
          break;
        }
      }
    }

    vector<string> active_args = splitwords;
    if (do_redirect) {
      active_args.resize(redirect_idx); // Strip out redirection tokens
    }

    if (active_args.empty()) exit(0);

    if (do_redirect) {
      int file_fd = open(redirect_file.c_str(), open_flags, 0644);
      if (file_fd < 0) {
        perror("open failed");
        exit(1);
      }
      dup2(file_fd, target_fd); // Overwrites only the child's stdout
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
    exit(0); // Safely terminate the child; the OS drops and closes all file descriptors
  } 
  else { // PARENT PROCESS INNER LOGIC
    int status;
    waitpid(pid, &status, 0); // Block until child exits cleanly
  }
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (1) {
    cout << "$ ";
    string command;
    if (!getline(cin, command)) break;
    vector<string> splitcommand;
    splitwords(command, splitcommand);
    executeCommand(splitcommand);
  }
  return 0;
}
