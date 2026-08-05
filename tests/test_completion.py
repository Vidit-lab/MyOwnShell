#!/usr/bin/env python3
"""Tab-completion and line-editing tests for myshell.

These need a real terminal: the line editor only enters raw mode when stdin is
a tty, so the piped-stdin suite in run_tests.sh can never reach any of this
code. Here the shell is driven under a PTY and its raw output inspected,
control characters and all.

Usage: test_completion.py <path-to-myshell>
"""
import os
import pty
import select
import sys
import tempfile
import time

SETTLE = 0.35  # seconds to wait for the shell to respond to each keystroke burst


def drive(binary, keys, cwd):
    """Type `keys` at the shell under a PTY; return everything it wrote."""
    pid, fd = pty.fork()
    if pid == 0:
        os.chdir(cwd)
        os.execv(binary, [binary])
        os._exit(127)

    out = b""

    def pump():
        nonlocal out
        deadline = time.time() + SETTLE
        while time.time() < deadline:
            ready, _, _ = select.select([fd], [], [], 0.05)
            if not ready:
                continue
            try:
                data = os.read(fd, 65536)
            except OSError:
                return
            if not data:
                return
            out += data

    time.sleep(SETTLE)
    for chunk in keys:
        os.write(fd, chunk)
        pump()

    try:
        os.write(fd, b"\x04")  # Ctrl-D to exit
    except OSError:
        pass
    pump()

    os.close(fd)
    try:
        os.waitpid(pid, 0)
    except ChildProcessError:
        pass
    return out


BELL = b"\x07"
passed = 0
failed = 0


def check(name, condition, transcript):
    global passed, failed
    if condition:
        print(f"ok   {name}")
        passed += 1
    else:
        print(f"FAIL {name}")
        print(f"  transcript: {transcript!r}")
        failed += 1


def main():
    if len(sys.argv) < 2:
        print("usage: test_completion.py <path-to-myshell>", file=sys.stderr)
        return 2
    shell = os.path.abspath(sys.argv[1])

    with tempfile.TemporaryDirectory() as work:
        for name in ("alpha.txt", "alpine.txt", "beta.txt"):
            open(os.path.join(work, name), "w").close()
        os.mkdir(os.path.join(work, "subdir"))

        completer = os.path.join(work, "completer.sh")
        with open(completer, "w") as f:
            f.write("#!/bin/sh\necho red\necho green\n")
        os.chmod(completer, 0o755)

        # ---- command-word completion ----

        t = drive(shell, [b"ech\t", b"hi\n"], work)
        check("unique command prefix completes and runs", b"echo hi" in t and b"\r\nhi\r\n" in t, t)

        t = drive(shell, [b"zzzzqq\t", b"\n"], work)
        check("command prefix with no match rings the bell", BELL in t, t)

        # ---- filename completion ----

        t = drive(shell, [b"cat bet\t", b"\n"], work)
        check("unique file prefix completes with a trailing space", b"cat beta.txt " in t, t)

        t = drive(shell, [b"cat subd\t", b"\n"], work)
        check("directory completion appends a slash, not a space", b"cat subdir/" in t, t)

        # "alpha.txt" and "alpine.txt" share exactly the prefix already typed,
        # so there is nothing to insert: bell first, list on the second Tab.
        t = drive(shell, [b"cat alp\t", b"\n"], work)
        check("ambiguous prefix rings the bell and inserts nothing",
              BELL in t and b"alpha.txt" not in t, t)

        t = drive(shell, [b"cat alp\t", b"\t", b"\n"], work)
        check("second Tab lists the candidates",
              b"alpha.txt  alpine.txt" in t, t)

        t = drive(shell, [b"cat alp\t", b"\t", b"\n"], work)
        check("listing redraws the prompt and the line",
              b"alpha.txt  alpine.txt\r\n$ cat alp" in t, t)

        t = drive(shell, [b"cat .\t", b"\n"], work)
        check("hidden entries are not offered without a leading dot", BELL in t, t)

        # ---- external completer ----

        t = drive(shell,
                  [b"complete -C " + completer.encode() + b" mycmd\n", b"mycmd \t", b"\t", b"\n"],
                  work)
        check("complete -C script supplies the candidates", b"red  green" in t, t)

        # ---- line editing ----

        t = drive(shell, [b"echo abcx\x7f\n"], work)
        check("backspace erases the last character", b"\x08 \x08" in t and b"\r\nabc\r\n" in t, t)

    print(f"\n{passed} passed, {failed} failed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
