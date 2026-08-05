#pragma once

#include <string>

#include <sys/types.h>

// A command launched with a trailing '&'. It stays in the job table until it
// exits *and* its completion has been reported, so the shell can print the
// "[1]+  Done" line at the next prompt rather than at some arbitrary moment.
struct BackgroundJob {
  pid_t pid;
  std::string command;
};

// Record a newly forked background command, print its "[id] pid" launch line,
// and return the job number assigned to it.
int register_background_job(pid_t pid, const std::string &command);

// Reap finished background jobs, printing a status line for each and dropping
// it from the table.
//
// `also_list_running` decides whether jobs that are still running are listed
// too: the `jobs` builtin wants the full table, while the sweep the REPL runs
// before each prompt only wants to announce what has finished.
void reap_jobs(bool also_list_running);
