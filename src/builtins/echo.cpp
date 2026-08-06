#include "builtins/builtins.hpp"

#include <iostream>

using namespace std;

void builtin_echo(const vector<string> &p) {
  for (size_t i = 1; i < p.size(); i++) {
    cout << p[i];
    if (i < p.size() - 1) cout << " ";
  }
  cout << endl;
}
