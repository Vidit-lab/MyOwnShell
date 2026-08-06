#include "completion/completion_specs.hpp"

#include <map>

using namespace std;

namespace {
// command -> completer script path, registered via `complete -C`.
map<string, string> completionSpecs;
}  // namespace

void register_completion_spec(const string &command, const string &script) {
  completionSpecs[command] = script;
}

void remove_completion_spec(const string &command) {
  completionSpecs.erase(command);
}

string lookup_completion_spec(const string &command) {
  auto it = completionSpecs.find(command);
  return it == completionSpecs.end() ? "" : it->second;
}
