#include "builtins/builtins.hpp"

#include "completion/completion_specs.hpp"

#include <iostream>

using namespace std;

// complete -C <script> <command>   register a completer
// complete -p <command>            print the registered completer
// complete -r <command>            remove it
void builtin_complete(const vector<string> &args) {
  if (args.size() >= 3 && args[1] == "-p") {
    const string &name = args[2];
    string script = lookup_completion_spec(name);
    if (!script.empty()) {
      cout << "complete -C '" << script << "' " << name << "\n";
    } else {
      cout << "complete: " << name << ": no completion specification\n";
    }
  } else if (args.size() >= 4 && args[1] == "-C") {
    register_completion_spec(args[3], args[2]);
  } else if (args.size() >= 3 && args[1] == "-r") {
    remove_completion_spec(args[2]);
  }
}
