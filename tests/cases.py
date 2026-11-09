"""Test data. Every expected value here was computed by hand from DOCS.md / README.md, not taken
from the assembler's output."""

# (name, source, expected instruction words as hex). The magic number is added by the runner.
KNOWN_ANSWERS = [
    ("set_imm",          "SET R1 #5",                                  ["30100005"]),
    ("set_reg",          "SET R1 R2",                                  ["10102000"]),
    ("add_reg",          "ADD R1 R2 R3",                               ["08123000"]),
    ("add_imm",          "ADD R1 R2 #1",                               ["28120001"]),
    ("sub_mul",          "SUB R6 R7 R8\nMUL R9 R10 #2",                ["09678000", "2c9a0002"]),
    ("div_mod",          "DIV R1 R2 R3\nMOD R4 R5 #9",                 ["0e123000", "2f450009"]),
    ("bitwise",          "AND R1 R2 R3\nOR R1 R2 #4\nXOR R3 R3 R3\nNOT R2 R1\nNOT R2 #1",
                         ["01123000", "22120004", "03333000", "00201000", "20200001"]),
    ("shifts_rotates",   "STL R3 R4 #15\nSTR R1 R2 R3\nRTL R1 R2 #1\nRTR R1 R2 R3",
                         ["2434000f", "05123000", "26120001", "07123000"]),
    ("neg_imm",          "NEG R2 #7",                                  ["2b200007"]),
    ("cmp_regs",         "CMP R1 R2",                                  ["11102000"]),
    ("max_values",       "SET R15 #65535",                             ["30f0ffff"]),
    ("load_hex",         "LOAD R5 0x0100",                             ["40500100"]),
    ("send_hex_letters", "SEND R1 0x00FF",                             ["411000ff"]),
    ("push",             "PUSH R3",                                    ["80300000"]),
    ("print_ops",        "PRINT R1\nPRINTC R2",                        ["90100000", "91200000"]),
    ("no_operand",       "RET\nEND\nCLR",                              ["a0000000", "bf000000", "b0000000"]),
    ("label_backward",   "@L\nGOTO @L",                                ["68000000"]),
    ("label_middle",     "SET R1 #1\n@L\nGOTO @L",                     ["30100001", "68000004"]),
    ("label_forward",    "GOTO @E\nSET R1 #1\n@E\nEND",                ["68000008", "30100001", "bf000000"]),
    ("all_branches",     "WEQ @a\nWNE @a\nWGT @a\nWLT @a\nWCY @a\nWOV @a\nWDZ @a\nCAL @a\n@a\nRET",
                         ["60000020", "61000020", "62000020", "63000020",
                          "64000020", "65000020", "66000020", "69000020", "a0000000"]),
    ("variables",        "LOAD R1 x\nLOAD R2 y\nSEND R1 x",            ["40100100", "40200102", "41100100"]),
    ("lone_R_variable",  "LOAD R1 R",                                  ["40100100"]),
    ("comments_blanks",  "; header\n\tSET R1 #5 ; trailing\n\n   END", ["30100005", "bf000000"]),
    ("empty_program",    "",                                           []),
]

# Error expectations: (line, column, kind). Columns are 1-based bytes, a tab counts as 1.
#   int          the first character of the offending token
#   (lo, hi)     any column from lo to hi: the token start up to one past its end. Used for lexical
#                errors, where the docs do not say whether the report points at the token start,
#                at the bad character, or just after the token
#   None         the position inside the line is not defined by the docs, so only column >= 1 is checked
# `kind` names a message in ERROR_KINDS, and "a|b" accepts either.
ERROR_KINDS = {
    "unknown_char":   r"Unknown character",
    "reg_range":      r"register number",
    "imm_range":      r"[Ii]mmediate value",
    "addr_range":     r"[Aa]ddress",
    "operand_fmt":    r"not in the correct format",
    "too_long":       r"too many operands",
    "undefined_label": r"not yet been defined",
    "dup_label":      r"already been defined",
    "label_extra":    r"extra oper",
    "first_token":    r"must either start with an OpCode or a Label",
    "imm_syntax":     r"not a correct immediate",
    "ident_name":     r"not named correctly",
    "addr_syntax":    r"address label",
}

# (name, source, [(line, col, kind), ...], ordered)
ERROR_CASES = [
    ("unknown_char",         "SET R1 ,",                 [(1, 8, "unknown_char")], True),
    ("stray_dollar",         "SET R1 #5 $",              [(1, 11, "unknown_char")], True),
    ("register_range",       "ADD R16 R1 R2",            [(1, 5, "reg_range")], True),
    ("register_huge",        "SET R99999999999999 #1",   [(1, (5, 20), "reg_range")], True),
    ("register_range_tab",   "\tADD R16 R1 R2",          [(1, 6, "reg_range")], True),
    ("immediate_range",      "SET R1 #65536",            [(1, 8, "imm_range")], True),
    ("immediate_huge",       "SET R1 #99999999999999",   [(1, (8, 23), "imm_range")], True),
    ("immediate_negative",   "SET R1 #-5",               [(1, None, "unknown_char|imm_syntax")], True),
    ("immediate_empty",      "SET R1 #",                 [(1, (8, 9), "imm_syntax")], True),
    ("bare_number",          "SET R1 5",                 [(1, 8, "ident_name")], True),
    ("hex_bad_digit",        "LOAD R1 0xZZ",             [(1, (9, 13), "addr_syntax")], True),
    ("hex_too_large",        "LOAD R1 0x10000",          [(1, 9, "addr_range")], True),
    ("send_immediate",       "SEND R1 #5",               [(1, 9, "operand_fmt")], True),
    ("goto_register",        "GOTO R1",                  [(1, 6, "operand_fmt")], True),
    ("missing_operand_3",    "ADD R1 R2",                [(1, None, "operand_fmt")], True),
    ("missing_operand_push", "PUSH",                     [(1, None, "operand_fmt")], True),
    ("extra_operand_end",    "END R1",                   [(1, 5, "operand_fmt")], True),
    ("too_many_operands",    "ADD R1 R2 R3 R4",          [(1, (14, 16), "too_long")], True),
    ("undefined_label",      "GOTO @nowhere",            [(1, 6, "undefined_label")], True),
    ("duplicate_label",      "@a\nEND\n@a",              [(3, 1, "dup_label")], True),
    ("label_with_operands",  "@a SET R1 #1",             [(1, None, "label_extra")], True),
    ("lowercase_opcode",     "set R1 #1",                [(1, 1, "first_token")], True),
    ("unknown_mnemonic",     "FOO R1",                   [(1, 1, "first_token")], True),
    ("several_errors",       "ADD R16 R1 R2\nSET R1 #5\nGOTO @x\nSET R1 #65536",
                             [(1, 5, "reg_range"), (3, 6, "undefined_label"), (4, 8, "imm_range")], True),
    ("parse_and_semantic",   "SET R1 ,\nGOTO @x\nSET R1 #5\nADD R16 R1 R2",
                             [(1, 8, "unknown_char"), (2, 6, "undefined_label"), (4, 5, "reg_range")], False),
]

# No golden snapshots are skipped any more: bug 4 is fixed, so the missing-operand output is deterministic.
GOLDEN_SKIP = set()
