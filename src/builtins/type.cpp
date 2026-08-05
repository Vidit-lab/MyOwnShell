#include "builtins/builtins.hpp"

#include "util/path.hpp"

#include <iostream>

using namespace std;

void builtin_type(const vector<string> &args) {
  if (args.size() < 2) return;
  string cmd = args[1];

  if (is_builtin(cmd)) {
    cout << cmd << " is a shell builtin\n";
    return;
  }

  string filepath = find_executable(cmd);
  if (!filepath.empty()) {
    cout << cmd << " is " << filepath << endl;
    return;
  }
  cout << cmd << ": not found\n";
}
