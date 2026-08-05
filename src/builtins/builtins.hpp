#pragma once

#include <string>
#include <vector>

using BuiltinFn = void (*)(const std::vector<std::string> &args);

// One builtin, and where in the fork it is allowed to run.
//
// Which side a builtin runs on is not a style choice — it decides what the
// builtin can actually do:
//
//   * `parent` runs in the shell process itself, before any fork, so it can
//     change state that has to survive the command: the working directory, the
//     completion table, the job table. It never sees a redirection or a pipe.
//   * `child` runs inside the forked stage, after redirections are applied and
//     pipe fds are wired up. That is what makes `pwd > f` and `echo hi | cat`
//     work — but anything it mutates dies with the fork.
//
// Either handler may be null. A builtin with no `child` handler that turns up
// in a pipeline stage falls through to the external-program lookup, exactly as
// an unknown command would.
struct Builtin {
  const char *name;
  BuiltinFn parent;
  BuiltinFn child;
};

// Table entry for `name`, or nullptr when `name` is not a builtin.
const Builtin *find_builtin(const std::string &name);

// Whether `name` is a builtin at all, wherever it runs. This is what `type`
// reports on.
bool is_builtin(const std::string &name);

// Every builtin name, for tab completion of the command word.
std::vector<std::string> builtin_names();

// ---- handlers, one per builtins/*.cpp ----

void builtin_echo(const std::vector<std::string> &args);
void builtin_pwd(const std::vector<std::string> &args);
void builtin_type(const std::vector<std::string> &args);
void builtin_cd(const std::vector<std::string> &args);
void builtin_complete(const std::vector<std::string> &args);
void builtin_jobs(const std::vector<std::string> &args);
void builtin_exit(const std::vector<std::string> &args);
void builtin_exit_child(const std::vector<std::string> &args);
