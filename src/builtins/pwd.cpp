#include "builtins/builtins.hpp"

#include <filesystem>
#include <iostream>

using namespace std;
namespace fs = std::filesystem;

void builtin_pwd(const vector<string> &) {
  cout << fs::current_path().string() << endl;
}
