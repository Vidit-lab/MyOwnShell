#include "util/path.hpp"

#include <cstdlib>
#include <sstream>
#include <unistd.h>

using namespace std;

vector<string> path_entries() {
  const char *env_path = getenv("PATH");
  string pathvar = env_path ? env_path : "";

  vector<string> entries;
  istringstream path_stream(pathvar);
  string pathsplit;
  while (getline(path_stream, pathsplit, ':')) {
    entries.push_back(pathsplit);
  }
  return entries;
}

string find_executable(const string &name) {
  for (const string &dir : path_entries()) {
    string filepath = dir + '/' + name;
    if (access(filepath.c_str(), X_OK) == 0) {
      return filepath;
    }
  }
  return "";
}
