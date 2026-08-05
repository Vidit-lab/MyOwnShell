#!/usr/bin/env bash
#
# Black-box regression tests for myshell.
#
# The shell is driven over a pipe (stdin is not a tty, so the line reader falls
# back to getline) and its stdout is compared against an expected transcript.
# Prompts are stripped and background PIDs normalised before comparison, so the
# expectations below stay readable.
#
# Usage: run_tests.sh [path-to-myshell]

set -uo pipefail

SHELL_BIN="${1:-}"
if [[ -z "$SHELL_BIN" ]]; then
  SHELL_BIN="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/build/myshell"
fi

if [[ ! -x "$SHELL_BIN" ]]; then
  echo "run_tests.sh: no shell binary at $SHELL_BIN" >&2
  exit 1
fi
# Tests run from a temp cwd, so the binary must be addressed absolutely.
SHELL_BIN="$(cd "$(dirname "$SHELL_BIN")" && pwd)/$(basename "$SHELL_BIN")"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

passed=0
failed=0

# Strip "$ " prompts and replace the pid in "[1] 12345" with a stable token.
run_shell() {
  (cd "$WORKDIR" && "$SHELL_BIN" 2>/dev/null) \
    | sed -E 's/\$ //g; s/^\[([0-9]+)\] [0-9]+$/[\1] PID/'
}

# check <name> <input> <expected-stdout>
check() {
  local name="$1" input="$2" expected="$3"
  local actual
  actual="$(printf '%s\n' "$input" | run_shell)"
  if [[ "$actual" == "$expected" ]]; then
    printf 'ok   %s\n' "$name"
    passed=$((passed + 1))
  else
    printf 'FAIL %s\n' "$name"
    printf '  expected: %q\n' "$expected"
    printf '  actual:   %q\n' "$actual"
    failed=$((failed + 1))
  fi
}

# ---- argument parsing / quoting ----

check "echo plain" \
  'echo hello world' \
  'hello world'

check "echo with no arguments prints a blank line" \
  'echo' \
  ''

check "echo collapses unquoted runs of spaces" \
  'echo a     b' \
  'a b'

check "echo single quotes preserve spaces" \
  "echo 'a     b'" \
  'a     b'

check "echo double quotes preserve spaces" \
  'echo "a     b"' \
  'a     b'

check "echo backslash escape outside quotes" \
  'echo before\   after' \
  'before  after'

check "echo backslash inside double quotes" \
  'echo "quote\"inside"' \
  'quote"inside'

check "echo backslash is literal inside single quotes" \
  "echo 'a\\nb'" \
  'a\nb'

check "adjacent quoted segments join into one word" \
  "echo 'foo'\"bar\"" \
  'foobar'

# ---- builtins ----

check "pwd reports the working directory" \
  'pwd' \
  "$WORKDIR"

check "cd changes directory" \
  "$(printf 'cd /usr\npwd')" \
  '/usr'

check "cd ~ goes home" \
  "$(printf 'cd ~\npwd')" \
  "$HOME"

check "cd to a missing directory reports an error" \
  'cd /nonexistent-dir-xyz' \
  'cd: /nonexistent-dir-xyz: No such file or directory'

check "type identifies a builtin" \
  'type echo' \
  'echo is a shell builtin'

check "type identifies an external program" \
  'type ls' \
  "ls is $(command -v ls)"

check "type reports unknown commands" \
  'type definitely_not_a_command' \
  'definitely_not_a_command: not found'

# ---- external programs ----

check "runs an external program with arguments" \
  'basename /a/b/c' \
  'c'

check "unknown command writes nothing to stdout" \
  'definitely_not_a_command arg' \
  ''

# ---- redirection ----

check "stdout truncating redirect" \
  "$(printf 'echo written > out.txt\ncat out.txt')" \
  'written'

check "stdout appending redirect" \
  "$(printf 'echo one > app.txt\necho two >> app.txt\ncat app.txt')" \
  "$(printf 'one\ntwo')"

check "1> is an alias for >" \
  "$(printf 'echo alias 1> alias.txt\ncat alias.txt')" \
  'alias'

check "stderr redirect leaves stdout clean" \
  "$(printf 'ls /nonexistent-xyz 2> err.txt\necho done')" \
  'done'

check "stderr redirect captures the message" \
  "$(printf 'ls /nonexistent-xyz 2> err2.txt\ncat err2.txt')" \
  "$(ls /nonexistent-xyz 2>&1 >/dev/null)"

check "builtin output can be redirected" \
  "$(printf 'pwd > pwd.txt\ncat pwd.txt')" \
  "$WORKDIR"

# ---- pipelines ----

check "two-stage pipeline" \
  'echo hello | wc -c' \
  '6'

check "three-stage pipeline" \
  "$(printf 'echo one > p.txt\necho two >> p.txt\necho three >> p.txt\ncat p.txt | grep t | wc -l')" \
  '2'

check "builtin as a pipeline producer" \
  'echo piped | cat' \
  'piped'

check "builtin as a pipeline consumer" \
  'echo ignored | pwd' \
  "$WORKDIR"

check "type works inside a pipeline" \
  'type echo | cat' \
  'echo is a shell builtin'

# ---- completion specs ----

check "complete -C registers, -p prints" \
  "$(printf 'complete -C /bin/true mycmd\ncomplete -p mycmd')" \
  "complete -C '/bin/true' mycmd"

check "complete -r removes a spec" \
  "$(printf 'complete -C /bin/true mycmd\ncomplete -r mycmd\ncomplete -p mycmd')" \
  'complete: mycmd: no completion specification'

check "complete -p on an unknown command" \
  'complete -p nosuchcmd' \
  'complete: nosuchcmd: no completion specification'

# ---- background jobs ----

check "background job prints its id, then reports completion" \
  "$(printf 'sleep 0.1 &\nsleep 0.4\necho after')" \
  "$(printf '[1] PID\n[1]+  %-24s%s\nafter' 'Done' 'sleep 0.1')"

check "jobs lists a running background job" \
  "$(printf 'sleep 1 &\njobs')" \
  "$(printf '[1] PID\n[1]+  %-24s%s' 'Running' 'sleep 1 &')"

# ---- exit ----

check "exit stops the shell" \
  "$(printf 'echo before\nexit\necho after')" \
  'before'

printf '\n%d passed, %d failed\n' "$passed" "$failed"
[[ $failed -eq 0 ]]
