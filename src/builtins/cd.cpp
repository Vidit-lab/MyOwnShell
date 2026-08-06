#include "builtins/builtins.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

void builtin_cd(const vector<string> &args) {
  if (args.size() < 2) return;

  string target = args[1];
  if (target == "~") {
    const char *home = getenv("HOME");
    target = home ? home : "";
  }

  if (fs::exists(target) && fs::is_directory(target)) {
    fs::current_path(target);
  } else {
    cout << "cd: " << args[1] << ": No such file or directory\n";
  }
}
