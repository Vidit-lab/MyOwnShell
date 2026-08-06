#include "exec/external.hpp"

#include "builtins/builtins.hpp"
#include "util/path.hpp"

#include <cstdlib>
#include <iostream>

#include <unistd.h>

using namespace std;

void externalProgram(const vector<string> &splitwords) {
  string cmd = splitwords[0];
  string filepath = find_executable(cmd);
  if (filepath.empty()) {
    cerr << cmd << ": not found\n";
    exit(1);
  }

  vector<char *> argv;
  for (auto &a : splitwords) {
    argv.push_back(const_cast<char *>(a.c_str()));
  }
  argv.push_back(nullptr);

  execv(filepath.c_str(), argv.data());
  perror("execv failed");
  exit(1);
}

void runInProcess(const vector<string> &args) {
  if (args.empty()) _exit(0);

  // Redirections and pipe fds are already in place, so a builtin with a child
  // handler runs here and its output flows wherever the caller pointed it.
  // Everything else — including builtins that only exist parent-side — falls
  // through to the PATH lookup.
  const Builtin *builtin = find_builtin(args[0]);
  if (builtin && builtin->child) builtin->child(args);
  else                           externalProgram(args);  // execs, or exits on not-found
  _exit(0);
}
