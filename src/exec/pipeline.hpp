#pragma once

#include <string>
#include <vector>

// Split a token list on '|' into its command stages:
//   ["cat", "f", "|", "wc"]  ->  {["cat", "f"], ["wc"]}
std::vector<std::vector<std::string>> split_on_pipe(const std::vector<std::string> &tokens);

// Run a pipeline: one forked child per stage, each stage's stdout wired to the
// next stage's stdin, then wait for them all. Builtins with a child handler run
// in-process inside their stage, so they can sit anywhere in the pipeline.
void runPipeline(const std::vector<std::string> &tokens);
