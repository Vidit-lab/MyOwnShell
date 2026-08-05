#pragma once

#include <string>
#include <vector>

// Run a single, non-pipeline command.
//
// Strips a trailing '&', runs parent-side builtins in place, and otherwise
// forks a child that applies any redirection before running the command. Waits
// for the child unless the command was backgrounded, in which case it is handed
// to the job table instead.
void executeCommand(const std::vector<std::string> &args);
