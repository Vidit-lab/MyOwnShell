#include "exec/executor.hpp"
#include "exec/pipeline.hpp"
#include "jobs/job_control.hpp"
#include "line/line_reader.hpp"
#include "parser/tokenizer.hpp"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// ---- Main loop ----

namespace {
const string kPrompt = "$ ";
}

int main() {
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  while (true) {
    reap_jobs(/*also_list_running=*/false);   // report+clear finished jobs first
    cout << kPrompt << std::flush;

    string command_line = read_line_raw(kPrompt);

    vector<string> splitcommand = tokenize(command_line);

    if (find(splitcommand.begin(), splitcommand.end(), "|") != splitcommand.end())
      runPipeline(splitcommand);
    else
      executeCommand(splitcommand);
  }
  return 0;
}
