#include "exec/executor.hpp"

#include "builtins/builtins.hpp"
#include "exec/external.hpp"
#include "jobs/job_control.hpp"
#include "parser/redirection.hpp"
#include "util/string_utils.hpp"

#include <cstdlib>
#include <iostream>

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

void executeCommand(const vector<string> &args) {
  if (args.empty()) return;

  bool background = (args.back() == "&");
  vector<string> splitwords(args.begin(), args.end() - (background ? 1 : 0));
  if (splitwords.empty()) return;

  string cmd = splitwords[0];

  // Parent-side builtins mutate state that must outlive this command — the
  // working directory, the completion table, the job table — so they run here
  // in the shell process instead of being forked away.
  const Builtin *builtin = find_builtin(cmd);
  if (builtin && builtin->parent) {
    builtin->parent(splitwords);
    return;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
    return;
  }

  if (pid == 0) {
    RedirectionConfig redir = parse_redirection(splitwords);

    vector<string> active_args = splitwords;
    if (redir.active) {
      active_args.resize(redir.operator_idx);
    }

    if (active_args.empty()) exit(0);

    if (redir.active) {
      int file_fd = open(redir.file.c_str(), redir.open_flags, 0644);
      if (file_fd < 0) {
        perror("open failed");
        exit(1);
      }
      dup2(file_fd, redir.target_fd);
      close(file_fd);
    }

    runInProcess(active_args);
  }
  else {
    if (background) {
      register_background_job(pid, join(splitwords, " "));
    } else {
      int status;
      waitpid(pid, &status, 0);
    }
  }
}
