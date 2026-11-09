#!/usr/bin/env python3
"""Test suite for the Skala assembler (v1 and v1.1). Python standard library only.

    python tests/run_tests.py --exe path/to/main.exe            run everything
    python tests/run_tests.py --exe ... --record                (re)record the golden snapshots
    python tests/run_tests.py --exe ... --only 'known/*'        run matching tests only
    python tests/run_tests.py --exe ... --fuzz 1000 --seed 7    more fuzzing, other seed

The assembler reads progs/demo.skasm and writes progs/demo.bin under --root. The runner backs up
both files first and restores them at the end. The binary starts with the magic number
0x3CDEC0DE followed by big-endian 32-bit instruction words.
"""
import argparse
import difflib
import fnmatch
import os
import random
import re
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import cases                      # noqa: E402
import known_bugs                 # noqa: E402
import skala_ref                  # noqa: E402

MAGIC = skala_ref.MAGIC
ERROR_LINE = re.compile(r"^Line: (\d+), Column: (\d+): (.*?)\s*$")


class RunResult:
    def __init__(self, rc, stdout, stderr, binary, timed_out):
        self.rc, self.stdout, self.stderr, self.binary, self.timed_out = rc, stdout, stderr, binary, timed_out
        self.errors = [(int(m.group(1)), int(m.group(2)), m.group(3))
                       for m in (ERROR_LINE.match(l) for l in stdout.splitlines()) if m]

    @property
    def crashed(self):
        return self.timed_out or self.rc not in (0, 1) or "unexpected error" in self.stderr

    @property
    def accepted(self):
        return self.binary is not None and not self.errors and not self.crashed

    @property
    def words(self):
        if self.binary is None or not self.binary.startswith(MAGIC):
            return None
        body = self.binary[len(MAGIC):]
        return [body[i:i + 4].hex() for i in range(0, len(body), 4)]


class Target:
    """Runs the assembler on source text. SkalaV2 will use another extension and no magic number."""

    def __init__(self, root, exe, source_name="demo.skasm", bin_name="demo.bin", timeout=10):
        self.root, self.exe, self.timeout = Path(root), Path(exe).resolve(), timeout
        self.src = self.root / "progs" / source_name
        self.out = self.root / "progs" / bin_name
        self.saved = {}

    def __enter__(self):
        (self.root / "progs").mkdir(exist_ok=True)
        for p in (self.src, self.out):
            self.saved[p] = p.read_bytes() if p.exists() else None
        return self

    def __exit__(self, *exc):
        for p, data in self.saved.items():
            if data is None:
                if p.exists():
                    p.unlink()
            else:
                p.write_bytes(data)

    def run(self, source):
        data = source if isinstance(source, bytes) else source.encode("utf-8")
        self.src.write_bytes(data)
        if self.out.exists():
            self.out.unlink()
        try:
            p = subprocess.run([str(self.exe)], cwd=str(self.root), stdin=subprocess.DEVNULL,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=self.timeout)
            rc, out, err, timed_out = p.returncode, p.stdout.decode("utf-8", "replace"), p.stderr.decode("utf-8", "replace"), False
        except subprocess.TimeoutExpired:
            rc, out, err, timed_out = None, "", "", True
        binary = self.out.read_bytes() if self.out.exists() else None
        return RunResult(rc, out, err, binary, timed_out)


def detect_prog_name(root):
    """The assembler reads progs/<prog_name>.skasm. Read prog_name from assembler.hpp, default 'demo'."""
    hpp = Path(root) / "src" / "assembler" / "assembler.hpp"
    try:
        m = re.search(r'prog_name\s*=\s*"([^"]+)"', hpp.read_text(errors="replace"))
        if m:
            return m.group(1)
    except OSError:
        pass
    return "demo"


def preflight(t):
    """Run one tiny program and make sure the assembler really processed it. Returns a message on
    failure, or None. Without this, a wrong path shows up as dozens of confusing test failures."""
    res = t.run("SET R1 #5\nEND\n")
    if res.words == ["30100005", "bf000000"] and not res.errors:
        res = t.run("ADD R16 R1 R2\n")
        if res.errors and res.binary is None:
            return None
        why = "an invalid probe program was not reported as an error"
    else:
        why = "a valid probe program (SET R1 #5 / END) did not produce the expected binary"
    out = res.stdout.splitlines()
    shown = "\n".join("    " + l for l in out[:8]) or "    (nothing)"
    msg = [f"PREFLIGHT FAILED: {why}.",
           f"  assembler: {t.exe}",
           f"  runner wrote: {t.src}",
           f"  exit code: {res.rc}, binary written: {res.binary is not None}",
           f"  first lines of the assembler's output:\n{shown}"]
    if res.stderr.strip():
        msg.append("  stderr: " + res.stderr.strip().splitlines()[0])
    searched = next((l for l in out if "Path searched" in l), None)
    if any("File not found" in l for l in out):
        msg.append("  The assembler could not find its input file.")
        if searched:
            msg.append("  " + searched.strip())
        msg.append("  Fix: pass --root <folder that contains the progs folder the assembler uses>,")
        msg.append("  or --prog-name <name> if prog_name in assembler.hpp is not '" + t.src.stem + "'.")
    elif not out:
        msg.append("  The assembler printed nothing. Check that --exe is the assembler build.")
    else:
        msg.append("  The assembler ran, but not on the file the runner wrote. Either prog_name in assembler.hpp is not")
        msg.append("  '" + t.src.stem + "' (use --prog-name), or the build reads a different progs folder (use --root).")
    return "\n".join(msg)


# ------------------------------------------------------------------------------------------------
class Report:
    def __init__(self, verbose):
        self.rows, self.verbose = [], verbose

    def add(self, name, ok, detail=""):
        status = "pass" if ok else "fail"
        bugs = []
        for pattern, ids in known_bugs.KNOWN.items():
            if fnmatch.fnmatch(name, pattern):
                bugs = ids
        if bugs:
            status = "known" if status == "fail" else "xpass"
        elif name in known_bugs.PLATFORM_DEPENDENT:
            bugs = known_bugs.PLATFORM_DEPENDENT[name]
            status = "known" if status == "fail" else "pass"
        self.rows.append((name, status, detail, bugs))

    def show(self):
        for name, status, detail, bugs in self.rows:
            if status == "pass" and not self.verbose:
                continue
            tag = {"pass": "PASS ", "fail": "FAIL ", "known": "KNOWN", "xpass": "XPASS"}[status]
            extra = f"  [bug {','.join(map(str, bugs))}]" if bugs else ""
            print(f"{tag} {name}{extra}")
            if detail and status in ("fail", "known", "xpass"):
                for line in detail.rstrip().splitlines()[:12]:
                    print("        " + line)
        counts = {s: sum(1 for r in self.rows if r[1] == s) for s in ("pass", "fail", "known", "xpass")}
        print("\n" + "-" * 70)
        print(f"{len(self.rows)} tests: {counts['pass']} passed, {counts['fail']} failed, "
              f"{counts['known']} known-bug failures, {counts['xpass']} now passing")
        by_bug = {}
        for name, status, _, bugs in self.rows:
            if status == "known":
                for b in bugs:
                    by_bug.setdefault(b, []).append(name)
        for b in sorted(by_bug):
            print(f"  bug {b}: {len(by_bug[b])} failing - {known_bugs.BUGS.get(b, '')}")
        if counts["xpass"]:
            print("  XPASS tests now pass: remove them from known_bugs.py")
        return counts

    def exit_code(self, strict):
        bad = sum(1 for r in self.rows if r[1] in ("fail", "xpass") or (strict and r[1] == "known"))
        return 1 if bad else 0


def fmt_errors(errors):
    return "\n".join(f"{l}:{c}: {m}" for l, c, m in errors) or "(none)"


def snapshot(res):
    if res.crashed:
        return "crashed: yes\n"
    words = res.words
    binary = "none" if res.binary is None else (res.binary.hex() if words is None else " ".join(words) or "(magic only)")
    return f"binary: {binary}\nerrors:\n" + "".join(f"  {l}:{c}: {m}\n" for l, c, m in res.errors)


# ------------------------------------------------------------------------------------------------
def check_known(t, rep, sel):
    for name, src, words in cases.KNOWN_ANSWERS:
        if not sel(f"known/{name}"):
            continue
        res = t.run(src)
        ok = res.accepted and res.words == words
        detail = f"source: {src!r}\nexpected: {words}\ngot:      {res.words}\nerrors:   {fmt_errors(res.errors)}"
        if res.crashed:
            detail += f"\nthe assembler crashed (exit code {res.rc})"
        rep.add(f"known/{name}", ok, detail)


def check_programs(t, rep, sel):
    for path in sorted((HERE / "programs").glob("*.skasm")):
        name = f"ref/{path.stem}"
        if not sel(name):
            continue
        src = path.read_text()
        try:
            want = skala_ref.assemble(src)
        except skala_ref.RefError as e:
            rep.add(name, False, f"SUITE BUG: the reference rejects this program: {e}")
            continue
        res = t.run(src)
        detail = f"errors: {fmt_errors(res.errors)}\n"
        if res.binary is not None and res.binary != want:
            a, b = res.words or [], [want[i:i + 4].hex() for i in range(4, len(want), 4)]
            diff = [f"word {i}: assembler {x} reference {y}" for i, (x, y) in enumerate(zip(a, b)) if x != y]
            detail += f"{len(a)} words from the assembler, {len(b)} from the reference\n" + "\n".join(diff[:6])
        rep.add(name, res.accepted and res.binary == want, detail)


def error_matches(got, want, ordered):
    """Compare reported errors with expectations. Returns (ok, explanation)."""
    if len(got) != len(want):
        return False, f"expected {len(want)} error(s), got {len(got)}"
    pairs = list(zip(got, want)) if ordered else None
    if not ordered:
        remaining = list(want)
        pairs = []
        for g in got:
            hit = next((w for w in remaining if w[0] == g[0] and (w[1] is None or w[1] == g[1])), None)
            hit = hit or (remaining[0] if remaining else None)
            if hit in remaining:
                remaining.remove(hit)
            pairs.append((g, hit))
    for (gl, gc, gm), w in pairs:
        wl, wc, kinds = w
        lo, hi = wc if isinstance(wc, tuple) else (wc, wc)
        if gl != wl or gc < 1 or (lo is not None and not lo <= gc <= hi):
            shown = "*" if wc is None else (f"{lo}-{hi}" if lo != hi else lo)
            return False, f"expected {wl}:{shown}, got {gl}:{gc} ({gm})"
        if not any(re.search(cases.ERROR_KINDS[k], gm) for k in kinds.split("|")):
            return False, f"at {gl}:{gc} expected a '{kinds}' message, got: {gm}"
    return True, ""


def check_errors(t, rep, sel):
    for name, src, want, ordered in cases.ERROR_CASES:
        if not sel(f"error/{name}"):
            continue
        res = t.run(src)
        ok, why = error_matches(res.errors, want, ordered) if not res.crashed else (False, f"the assembler crashed (exit code {res.rc})")
        if ok and res.binary is not None:
            ok, why = False, "a binary was written although the program has errors"
        rep.add(f"error/{name}", ok, f"source: {src!r}\n{why}\nreported:\n{fmt_errors(res.errors)}")


def check_exit_codes(t, rep, sel):
    good = t.run("SET R1 #1\nEND")
    bad = t.run("ADD R16 R1 R2")
    if sel("exit/valid_program_returns_0"):
        rep.add("exit/valid_program_returns_0", good.rc == 0, f"exit code {good.rc}")
    if sel("exit/invalid_program_returns_1"):
        rep.add("exit/invalid_program_returns_1", bad.rc == 1, f"exit code {bad.rc}, expected 1")
    if sel("exit/no_binary_on_error"):
        rep.add("exit/no_binary_on_error", bad.binary is None, "a binary was written for an invalid program")


def variants(src):
    yield "crlf", src.replace("\n", "\r\n")
    yield "no_final_newline", src.rstrip("\n")
    yield "trailing_spaces", "\n".join(l + "   " for l in src.split("\n"))
    yield "tabs_between_tokens", "\n".join(l.replace(" ", "\t") for l in src.split("\n"))


def check_line_endings(t, rep, sel):
    for path in sorted((HERE / "programs").glob("*.skasm")):
        base_name = f"lineend/{path.stem}"
        if not sel(base_name):
            continue
        src = path.read_text()
        base = t.run(src)
        problems = []
        for vname, vsrc in variants(src):
            r = t.run(vsrc)
            if r.crashed or r.binary != base.binary or [e[2] for e in r.errors] != [e[2] for e in base.errors]:
                problems.append(f"variant '{vname}' differs from the plain file")
        rep.add(base_name, not problems and not base.crashed, "\n".join(problems))


def check_golden(t, rep, sel, record):
    gdir = HERE / "golden"
    items = [(f"known_{n}", s) for n, s, _ in cases.KNOWN_ANSWERS]
    items += [(f"error_{n}", s) for n, s, _, _ in cases.ERROR_CASES]
    items += [(f"program_{p.stem}", p.read_text()) for p in sorted((HERE / "programs").glob("*.skasm"))]
    if record:
        gdir.mkdir(exist_ok=True)
    elif not gdir.exists():
        return False
    for name, src in items:
        test = f"golden/{name}"
        if name in cases.GOLDEN_SKIP or (not sel(test) and not record):
            continue
        snap = snapshot(t.run(src))
        f = gdir / f"{name}.txt"
        if record:
            f.write_text(snap)
            continue
        old = f.read_text() if f.exists() else "(no snapshot)\n"
        diff = "".join(difflib.unified_diff(old.splitlines(True), snap.splitlines(True), "recorded", "now", n=0))
        rep.add(test, old == snap, diff)
    return True


# ---- differential fuzzing against the reference ---------------------------------------------
OPS = sorted(skala_ref.OPCODES)
VOCAB = (["R0", "R1", "R5", "R15", "R16", "R99", "R", "#0", "#7", "#65535", "#65536", "#-1", "0x0", "0xFF",
          "0x0100", "0xFFFF", "0x10000", "@a", "@b", "@zz", "x", "y", "my_var", "add", "set"])
JUNK = ["$", ",", "!", "(", ")", "R1,", "#", "@", "0x", "5", "a-b"]


def random_line(rng):
    op = rng.choice(OPS)
    n = rng.choice([0, 1, 1, 2, 2, 3, 3, 4])
    return " ".join([op] + [rng.choice(VOCAB) for _ in range(n)])


def mutate(rng, src):
    lines = [l.split(";", 1)[0].split() for l in src.split("\n")]
    for _ in range(rng.randint(1, 3)):
        op = rng.choice(["del_tok", "dup_tok", "swap_tok", "repl_tok", "ins_tok", "junk",
                         "del_line", "dup_line", "swap_lines", "ins_line"])
        nonempty = [i for i, l in enumerate(lines) if l]
        if op == "ins_line" or not nonempty:
            lines.insert(rng.randint(0, len(lines)), random_line(rng).split())
            continue
        i = rng.choice(nonempty)
        l = lines[i]
        if op == "del_tok":
            del l[rng.randrange(len(l))]
        elif op == "dup_tok":
            j = rng.randrange(len(l)); l.insert(j, l[j])
        elif op == "swap_tok" and len(l) > 1:
            a, b = rng.sample(range(len(l)), 2); l[a], l[b] = l[b], l[a]
        elif op == "repl_tok":
            l[rng.randrange(len(l))] = rng.choice(VOCAB + OPS)
        elif op == "ins_tok":
            l.insert(rng.randint(0, len(l)), rng.choice(VOCAB))
        elif op == "junk":
            l.insert(rng.randint(0, len(l)), rng.choice(JUNK))
        elif op == "del_line":
            del lines[i]
        elif op == "dup_line":
            lines.insert(i, list(l))
        elif op == "swap_lines" and len(lines) > 1:
            j = rng.randrange(len(lines)); lines[i], lines[j] = lines[j], lines[i]
    return "\n".join(" ".join(l) for l in lines)


def check_fuzz(t, rep, sel, count, seed):
    if count <= 0 or not any(sel(n) for n in ("fuzz/no_crash", "fuzz/accepts_valid", "fuzz/matches_reference",
                                              "fuzz/rejects_invalid", "fuzz/error_positions")):
        return
    corpus = [s for _, s, _ in cases.KNOWN_ANSWERS if s] + [p.read_text() for p in sorted((HERE / "programs").glob("*.skasm"))]
    rng = random.Random(seed)
    fails = {k: [] for k in ("no_crash", "accepts_valid", "matches_reference", "rejects_invalid", "error_positions")}
    nvalid = 0
    for i in range(count):
        src = mutate(rng, rng.choice(corpus))
        res = t.run(src)
        label = f"iteration {i}: {src!r}"
        if res.crashed:
            fails["no_crash"].append(f"{label} -> exit code {res.rc}")
            continue
        try:
            want, ref_ok = skala_ref.assemble(src), True
        except skala_ref.RefError as e:
            want, ref_ok = None, False
        for l, c, m in res.errors:
            if l < 1 or c < 1:
                fails["error_positions"].append(f"{label} -> error at {l}:{c}")
                break
        if ref_ok:
            nvalid += 1
            if not res.accepted:
                fails["accepts_valid"].append(f"{label} -> rejected: {fmt_errors(res.errors)}")
            elif res.binary != want:
                fails["matches_reference"].append(f"{label} -> binary differs from the reference")
        elif res.accepted:
            fails["rejects_invalid"].append(f"{label} -> accepted, the reference says: {e}")
    for k, items in fails.items():
        extra = f"seed {seed}, {count} programs ({nvalid} valid)"
        rep.add(f"fuzz/{k}", not items, f"{len(items)} failing, {extra}\n" + "\n".join(x[:200] for x in items[:5]))


# ------------------------------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--root", default=str(HERE.parent), help="Skala repository root (default: parent of tests/)")
    ap.add_argument("--exe", help="assembler executable (default: <root>/main.exe or <root>/main)")
    ap.add_argument("--prog-name", help="input name used by the assembler (default: prog_name from assembler.hpp, else demo)")
    ap.add_argument("--no-preflight", action="store_true", help="skip the start-up check")
    ap.add_argument("--only", default="*", help="fnmatch pattern for test names")
    ap.add_argument("--fuzz", type=int, default=300, help="number of fuzz programs (0 disables)")
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--record", action="store_true", help="record golden snapshots from the current build")
    ap.add_argument("--strict", action="store_true", help="count known-bug failures as failures")
    ap.add_argument("-v", "--verbose", action="store_true", help="also list passing tests")
    a = ap.parse_args()

    root = Path(a.root).resolve()
    exe = Path(a.exe) if a.exe else next((p for p in (root / "main.exe", root / "main") if p.exists()), None)
    if exe is None or not exe.exists():
        sys.exit("assembler executable not found: build it and pass --exe")
    sel = lambda name: fnmatch.fnmatch(name, a.only)

    name = a.prog_name or detect_prog_name(root)
    with Target(root, exe, source_name=name + ".skasm", bin_name=name + ".bin") as t:
        print(f"assembler: {exe}\ninput:     {t.src}")
        if not a.no_preflight:
            problem = preflight(t)
            if problem:
                print(problem)
                return 2
        if a.record:
            check_golden(t, None, sel, True)
            print(f"golden snapshots recorded in {HERE / 'golden'}")
            return 0
        rep = Report(a.verbose)
        check_known(t, rep, sel)
        check_programs(t, rep, sel)
        check_errors(t, rep, sel)
        check_exit_codes(t, rep, sel)
        check_line_endings(t, rep, sel)
        check_fuzz(t, rep, sel, a.fuzz, a.seed)
        had_golden = check_golden(t, rep, sel, False)
    rep.show()
    if not had_golden:
        print("  (no golden snapshots found: run with --record on a build you trust)")
    return rep.exit_code(a.strict)


if __name__ == "__main__":
    sys.exit(main())
