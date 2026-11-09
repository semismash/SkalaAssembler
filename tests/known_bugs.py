"""Failures that are expected until the matching bug is fixed. Empty: all five bugs are fixed.

BUGS maps a bug number to a description. KNOWN maps a test name (or fnmatch pattern) to the bug
numbers that explain its failure. The runner reports a failing test listed here as KNOWN and a
passing test listed here as XPASS, which means "the fix works, delete this entry".
"""

BUGS = {
    1: "hex addresses are parsed as decimal (0x10 encodes as 10, 0xFF is rejected)",
    2: "a label line emits a zero word, so label addresses point before the real instruction",
    3: "a lone 'R' (as in 'LOAD R1 R') aborts the assembler",
    4: "a missing operand reads an empty optional (undefined behavior): line 0 column 0, or an abort, depending on the build",
    5: "exit code is 0 even when the program has errors",
}

KNOWN = {}

PLATFORM_DEPENDENT = {}
