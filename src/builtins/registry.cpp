#include "builtins/builtins.hpp"

using namespace std;

namespace {

// Single source of truth for what a builtin is. `type`, tab completion, and
// both dispatch sites all derive from this one table — adding a builtin means
// adding a row here and nothing else.
const vector<Builtin> kBuiltins = {
    // name        parent handler     child handler
    {"echo",       nullptr,           builtin_echo},
    {"exit",       builtin_exit,      builtin_exit_child},
    {"pwd",        nullptr,           builtin_pwd},
    {"cd",         builtin_cd,        nullptr},
    {"type",       nullptr,           builtin_type},
    {"complete",   builtin_complete,  nullptr},
    {"jobs",       builtin_jobs,      nullptr},
};

}  // namespace

const Builtin *find_builtin(const string &name) {
  for (const Builtin &builtin : kBuiltins) {
    if (name == builtin.name) return &builtin;
  }
  return nullptr;
}

bool is_builtin(const string &name) {
  return find_builtin(name) != nullptr;
}

vector<string> builtin_names() {
  vector<string> names;
  names.reserve(kBuiltins.size());
  for (const Builtin &builtin : kBuiltins) names.push_back(builtin.name);
  return names;
}
