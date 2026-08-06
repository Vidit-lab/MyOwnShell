#include "exec/pipeline.hpp"

#include "exec/external.hpp"

#include <cstdio>

#include <sys/wait.h>
#include <unistd.h>

using namespace std;

vector<vector<string>> split_on_pipe(const vector<string> &tokens) {
  vector<vector<string>> stages;
  vector<string> cur;
  for (const auto &t : tokens) {
    if (t == "|") { stages.push_back(cur); cur.clear(); }
    else cur.push_back(t);
  }
  stages.push_back(cur);
  return stages;
}

void runPipeline(const vector<string> &tokens) {
  vector<vector<string>> stages = split_on_pipe(tokens);
  int n = (int)stages.size();

  int prev_read = -1;  // read end feeding the current stage's stdin
  vector<pid_t> pids;

  for (int i = 0; i < n; ++i) {
    bool has_next = (i < n - 1);
    int fds[2] = {-1, -1};
    if (has_next && pipe(fds) < 0) { perror("pipe failed"); break; }

    pid_t pid = fork();
    if (pid == 0) {
      if (prev_read != -1) dup2(prev_read, STDIN_FILENO);  // stdin from previous stage
      if (has_next)        dup2(fds[1], STDOUT_FILENO);    // stdout to next stage
      if (prev_read != -1) close(prev_read);
      if (has_next) { close(fds[0]); close(fds[1]); }
      runInProcess(stages[i]);  // builtin in-process, else exec; never returns
    }

    pids.push_back(pid);
    if (prev_read != -1) close(prev_read);  // parent is done with the incoming read end
    if (has_next) {
      close(fds[1]);      // parent never writes
      prev_read = fds[0];  // hand the read end to the next stage
    }
  }

  for (pid_t p : pids) waitpid(p, nullptr, 0);
}
