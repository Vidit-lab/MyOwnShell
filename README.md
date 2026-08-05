# MyOwnShell

A POSIX-style shell written from scratch in C++17 — no readline, no libedit, no
parser generator.

## Features

- Quote-aware tokenizer: single quotes, double quotes, backslash escapes
- Builtins: `echo`, `pwd`, `cd`, `type`, `exit`, `complete`, `jobs`
- External programs resolved on `PATH` via `fork` + `execv`
- Redirection: `>`, `>>`, `1>`, `1>>`, `2>`, `2>>`
- Pipelines of any length, with builtins usable at any stage
- Background jobs (`cmd &`) and a `jobs` listing with `+`/`-` markers
- Tab completion for commands, filenames, and `complete -C` scripts
- Raw-mode line editor built on termios

## Build

```sh
cmake -S . -B build
cmake --build build
./build/myshell
```

Requires a C++17 compiler and CMake 3.16+.

## Test

```sh
ctest --test-dir build --output-on-failure
```

`tests/run_tests.sh` drives the shell over a pipe; `tests/test_completion.py`
drives it under a PTY, which is the only way to reach raw mode and tab
completion. Both are black-box against the real binary.

## Layout

```
src/
├── main.cpp        entry point
├── core/           the REPL and command routing
├── parser/         tokenizer and redirection parsing
├── exec/           process creation: builtins, externals, pipelines
├── builtins/       one file per builtin, plus the registry
├── jobs/           background job table and reaping
├── line/           raw-mode terminal handling and the line editor
├── completion/     the tab-completion engines
└── util/           PATH search and string helpers
```

Dependencies run downward and never back up. Each builtin is a row in
`builtins/registry.cpp` with a parent handler, a child handler, or both:
parent handlers run in the shell process (`cd`, `complete`, `jobs`), child
handlers run in the forked stage where redirections and pipes apply (`echo`,
`pwd`, `type`).
