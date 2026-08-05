#include "completion/external_completer.hpp"

#include <cstdlib>
#include <sstream>

#include <sys/wait.h>
#include <unistd.h>

using namespace std;

vector<string> run_completer(const string &script, const string &cmd_name, const string &cur_word,
                             const string &prev_word, const string &comp_line, size_t comp_point) {
  int fds[2];
  if (pipe(fds) < 0) return {};

  pid_t pid = fork();
  if (pid < 0) { close(fds[0]); close(fds[1]); return {}; }

  if (pid == 0) {
    dup2(fds[1], STDOUT_FILENO);
    close(fds[0]);
    close(fds[1]);
    setenv("COMP_LINE", comp_line.c_str(), 1);
    setenv("COMP_POINT", to_string(comp_point).c_str(), 1);
    vector<char *> argv = {
      const_cast<char*>(script.c_str()),
      const_cast<char*>(cmd_name.c_str()),
      const_cast<char*>(cur_word.c_str()),
      const_cast<char*>(prev_word.c_str()),
      nullptr
    };
    execv(script.c_str(), argv.data());
    _exit(127);
  }

  close(fds[1]);
  string out;
  char buf[4096];
  ssize_t n;
  while ((n = read(fds[0], buf, sizeof(buf))) > 0) out.append(buf, n);
  close(fds[0]);
  waitpid(pid, nullptr, 0);

  vector<string> lines;
  istringstream iss(out);
  string line;
  while (getline(iss, line)) {
    if (!line.empty()) lines.push_back(line);
  }
  return lines;
}
