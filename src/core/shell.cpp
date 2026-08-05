#include "core/shell.hpp"

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

namespace {
const string kPrompt = "$ ";
}

void run_repl() {
  while (true) {
    reap_jobs(/*also_list_running=*/false);  // report+clear finished jobs first
    cout << kPrompt << std::flush;

    // The reader needs the prompt to redraw the line after a completion listing.
    string command_line = read_line_raw(kPrompt);

    vector<string> tokens = tokenize(command_line);

    if (find(tokens.begin(), tokens.end(), "|") != tokens.end())
      runPipeline(tokens);
    else
      executeCommand(tokens);
  }
}
