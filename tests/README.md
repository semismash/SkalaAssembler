# Skala assembler tests

Python standard library only. Put this folder in the Skala repository root (as `tests/`) and run:

```
python tests/run_tests.py --exe main.exe          # Windows
python tests/run_tests.py --exe ./main             # Linux
```

Useful options: `--only 'known/*'` (one group), `--fuzz 1000 --seed 7` (more fuzzing), `-v` (list passing
tests), `--record` (re-record the golden snapshots), `--strict` (count known-bug failures as failures).

The assembler reads `progs/demo.skasm` and writes `progs/demo.bin`. The runner backs up both files
before it starts and restores them at the end. Binaries start with the magic number `0x3CDEC0DE`,
followed by big-endian 32-bit instruction words.

## If almost everything fails

The runner starts with a preflight check: it assembles `SET R1 #5` / `END` and stops with a message if the
assembler did not process it. The usual causes are:

* `prog_name` in `src/assembler/assembler.hpp` is not `demo`. The runner reads it from that file
  automatically, or you can pass `--prog-name <name>`.
* The assembler was built with absolute paths (for example from an IDE task), so it reads the `progs`
  folder of another checkout. Pass `--root` with the repository that holds that `progs` folder.

## What is tested

| Group | What it checks | Expected value comes from |
| --- | --- | --- |
| `known/` | 24 small programs and their exact words | computed by hand from `DOCS.md` |
| `ref/` | each program in `programs/` against the reference encoder | `skala_ref.py` (written from `DOCS.md` and the README, shares no code with the C++) |
| `error/` | line, column and message of each reported error, and that no binary is written | written by hand in `cases.py` |
| `exit/` | exit code 0 for valid programs, 1 for invalid ones | fixed |
| `lineend/` | CRLF, missing final newline, trailing spaces and tabs give identical output | the plain file's own output |
| `fuzz/` | random token-level mutations compared with the reference: no crash, same accept or reject decision, same binary, sane error positions | `skala_ref.py` |
| `golden/` | output of every test program compared with the saved snapshot in `golden/` | recorded from the build before any fix |

A golden snapshot records what the code does, not what is correct. It only tells you that something changed.
After a fix, the snapshots the fix was meant to change will fail. Check the diff, then run `--record`.

## Reading the output

* `PASS`   the test passed.
* `FAIL`   unexpected failure: a new bug, or a mistake in the suite.
* `KNOWN`  failure listed in `known_bugs.py`, with the bug number it belongs to.
* `XPASS`  a test listed in `known_bugs.py` now passes. The fix works: delete its entry.

The exit code is 1 if there is any `FAIL` or `XPASS`.

## Column convention

Columns are 1-based bytes and a tab counts as 1. Semantic errors must point at the first character of
the offending token. For lexical errors (a bad character, an oversized number) the docs do not say whether
the column points at the token start, the bad character or just past the token, so the tests accept any
column from the token start to one past its end.

## SkalaV2

SkalaV2 uses `.s` files and writes raw 16-bit little-endian words with no magic number. `Target` already
takes the source and output file names as parameters. A second profile for the word format will be added
when SkalaV2 has something to test.
