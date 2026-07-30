#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_set>
#include <cstdlib>
#include <filesystem>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
const unordered_set<string> listCommands = {"type", "echo", "exit", "pwd", "cd"};

void exitCommand() { exit(0); }

void echoCommand(vector<string> &p) {
  for (size_t i = 1; i < p.size(); i++) {
    cout << p[i];
    if (i < p.size() - 1) {
      cout << " ";
    }
  }
  cout << endl;
}

void pwdCommand() {
  cout << std::filesystem::current_path().string() << endl;
}

// Builtin handler for the "cd" command (handling all paths)
void cdCommand(const vector<string> &splitwords) {
  if (splitwords.size() < 2) return;

  string target_str = splitwords[1];

  if (target_str == "~") {
    const char* home_env = getenv("HOME");
    target_str = home_env ? home_env : "";
  }

  std::filesystem::path target_dir = target_str;

  if (std::filesystem::exists(target_dir) && std::filesystem::is_directory(target_dir)) {
    std::filesystem::current_path(target_dir); 
  } else {
    cout << "cd: " << splitwords[1] << ": No such file or directory\n";
  }
}

void splitwords(string &command, vector<string> &splitted) {
  string current_arg = "";
  bool in_single_quotes = false;
  bool in_double_quotes = false;
  bool inside_word = false;

  for (size_t i = 0; i < command.length(); ++i) {
    char c = command[i];

    if (in_single_quotes) {
      if (c == '\'') {
        in_single_quotes = false;
      } else {
        current_arg += c;
      }
    } else if (in_double_quotes) {
      if (c == '"') {
        in_double_quotes = false;
      } else {
        current_arg += c; 
      }
    } else {
      
      if (c == '\'') {
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

  if (inside_word) {
    splitted.push_back(current_arg);
  }
}

void typeCommand(vector<string> &splitwords) {
  if (splitwords.size() < 2) return; // Guard clause against missing arguments
  string cmd = splitwords[1];
  if (listCommands.count(cmd)) {
    cout << cmd << " is a shell builtin\n";
    return;
  } else {
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
  }
  cout << cmd << ": not found\n";
}

void externalProgram(vector<string> &splitwords) {
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
        // Industry-standard casting for low-level system exec boundaries
        argv.push_back(const_cast<char*>(a.c_str()));
      }
      argv.push_back(nullptr);

      pid_t pid = fork();
      if (pid == 0) {
        execv(filepath.c_str(), argv.data());
        perror("execv failed");
        exit(1);
      } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
      } else {
        perror("fork failed");
      }
      return;
    }
  }
  cout << cmd << ": not found\n";
}

void commandEx(vector<string> &splitwords) {
  if (splitwords.empty()) return; // FIX: Protects against out-of-bounds crashes on blank enter
  string cmd = splitwords[0];
  if (cmd == "exit") {
    exitCommand();
  } else if (cmd == "echo") {
    echoCommand(splitwords);
  } else if (cmd == "type") {
    typeCommand(splitwords);
  } else if (cmd == "pwd") { 
    pwdCommand();
  } else if (cmd == "cd") { 
    cdCommand(splitwords);
  }else {
    externalProgram(splitwords);
  }
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (1) {
    cout << "$ ";
    string command;
    if (!getline(cin, command)) break; // Guard against EOF/Ctrl+D hanging loops
    vector<string> splitcommand;
    splitwords(command, splitcommand);
    commandEx(splitcommand);
  }

  return 0;
}
