#include "jobs/job_control.hpp"

#include <iostream>
#include <map>
#include <set>
#include <utility>

#include <sys/wait.h>

using namespace std;

namespace {

// Ordered by job number so listings come out in job order, and so the two
// highest ids (the +/- markers) are a rbegin() away.
map<int, BackgroundJob> jobTable;

// The two most-recently-started job ids (0 if absent), for the +/- markers.
pair<int, int> current_and_previous_ids() {
  int max_id = jobTable.empty() ? 0 : jobTable.rbegin()->first;
  int second_id = 0;
  if (jobTable.size() >= 2) {
    auto it = jobTable.rbegin();
    ++it;
    second_id = it->first;
  }
  return {max_id, second_id};
}

char job_marker(int id, int max_id, int second_id) {
  if (id == max_id) return '+';
  if (id == second_id) return '-';
  return ' ';
}

// One listing line. `running` picks the status text AND whether '&' is shown.
void print_job_line(int id, char marker, const string &command, bool running) {
  string field = running ? "Running" : "Done";
  if (field.size() < 24) field.append(24 - field.size(), ' ');
  cout << "[" << id << "]" << marker << "  " << field << command;
  if (running) cout << " &";
  cout << "\n";
}

set<int> poll_exited_jobs() {
  set<int> exited;
  for (const auto &[id, job] : jobTable) {
    int status;
    pid_t r = waitpid(job.pid, &status, WNOHANG);
    if (r == job.pid && WIFEXITED(status)) exited.insert(id);
  }
  return exited;
}

}  // namespace

int register_background_job(pid_t pid, const string &command) {
  // Recycle numbers: reuse freed slots instead of growing forever.
  int job_id = jobTable.empty() ? 1 : jobTable.rbegin()->first + 1;
  jobTable[job_id] = {pid, command};
  cout << "[" << job_id << "] " << pid << "\n";
  return job_id;
}

void reap_jobs(bool also_list_running) {
  set<int> exited = poll_exited_jobs();
  if (exited.empty() && !also_list_running) return;

  auto [max_id, second_id] = current_and_previous_ids();
  for (const auto &[id, job] : jobTable) {
    bool running = (exited.count(id) == 0);
    if (running && !also_list_running) continue;  // sweep: skip still-running
    print_job_line(id, job_marker(id, max_id, second_id), job.command, running);
  }

  for (int id : exited) jobTable.erase(id);
}
