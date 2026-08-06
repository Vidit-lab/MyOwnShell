#pragma once

#include <string>

// The `complete -C <script> <command>` table.
//
// A spec says "to complete arguments of <command>, run <script> and read its
// stdout". Registrations outlive the command that made them, so this table
// lives in the shell process and is only ever touched before a fork.

void register_completion_spec(const std::string &command, const std::string &script);

void remove_completion_spec(const std::string &command);

// Script registered for `command`, or "" when it has no spec. A registered
// script path is never empty, so "" is an unambiguous "not found".
std::string lookup_completion_spec(const std::string &command);
