#include "util/string_utils.hpp"

using namespace std;

string longest_common_prefix(const vector<string> &values) {
  if (values.empty()) return "";

  string prefix = values[0];
  for (size_t i = 1; i < values.size(); ++i) {
    size_t j = 0;
    while (j < prefix.size() && j < values[i].size() && prefix[j] == values[i][j]) ++j;
    prefix.resize(j);
    if (prefix.empty()) break;
  }
  return prefix;
}

string join(const vector<string> &parts, const string &separator) {
  string joined;
  for (size_t i = 0; i < parts.size(); ++i) {
    if (i) joined += separator;
    joined += parts[i];
  }
  return joined;
}
