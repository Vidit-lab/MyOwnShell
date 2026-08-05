# MyOwnShell

A POSIX-style shell written from scratch in C++17 — no readline, no libedit, no
parser generator. Everything from the tokenizer to the tab-completion engine is
hand-rolled against raw POSIX syscalls.

## Features

- **Quote-aware tokenizer** — single quotes, double quotes, and backslash
  escapes with the POSIX rules for each context
- **Builtins** — `echo`, `pwd`, `cd` (including `~`), `type`, `exit`,
  `complete`, `jobs`
- **External programs** — resolved by scanning `PATH`, run via `fork` + `execv`
- **Redirection** — `>`, `>>`, `1>`, `1>>`, `2>`, `2>>`
- **Pipelines** — arbitrary-length `a | b | c`, with builtins usable at any
  stage
- **Background jobs** — `cmd &`, job-number recycling, and `jobs` listing with
  bash's `+`/`-` current/previous markers
- **Tab completion** — builtins and `PATH` executables for the command word,
  filesystem completion for arguments, external completer scripts via
  `complete -C`, longest-common-prefix insertion and two-Tab listing
- **Raw-mode line editor** — termios-based, with backspace and Ctrl-D handling

## Building

```sh
cmake -S . -B build
cmake --build build
./build/myshell
```

Requires a C++17 compiler and CMake 3.16+.

## Testing

```sh
ctest --test-dir build --output-on-failure
```

or run the harness directly:

```sh
./tests/run_tests.sh ./build/myshell
```

The suite is black-box: it drives the built binary over a pipe and diffs the
transcript, so it exercises the real `fork`/`exec`/`dup2` paths rather than
mocking them.

## Layout

See [docs/architecture.md](docs/architecture.md) for the module map. In short:

```
src/
├── main.cpp        entry point
├── core/           the REPL and top-level command routing
├── parser/         tokenizer and redirection parsing
├── exec/           process creation: builtins, externals, pipelines
├── builtins/       one file per builtin, plus the registry
├── jobs/           background job table and reaping
├── line/           raw-mode terminal handling and the line editor
├── completion/     the tab-completion engines
└── util/           PATH search and small string helpers
```
