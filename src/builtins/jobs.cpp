#include "builtins/builtins.hpp"

#include "jobs/job_control.hpp"

using namespace std;

void builtin_jobs(const vector<string> &) {
  reap_jobs(/*also_list_running=*/true);
}
