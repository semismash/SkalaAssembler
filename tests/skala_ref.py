"""Reference encoder for the Skala ISA.

Written from DOCS.md and README.md only. It shares no code with the C++ assembler, so a
disagreement between the two points at a bug in one of them.

    assemble(source) -> bytes      magic number + big-endian 32-bit instruction words
    raises RefError(line, col, message) for the first invalid construct
"""
import re

MAGIC = bytes.fromhex("3cdec0de")          # 0x3CDEC0DE, big-endian (DOCS.md, "Addresses and Binary File")


class RefError(Exception):
    def __init__(self, line, col, message):
        super().__init__(f"{line}:{col}: {message}")
        self.line, self.col, self.message = line, col, message


# ---- DOCS.md: instruction types (first 3 bits) and 5-bit instruction codes -------------------
TYPE = {"R": 0b000, "I": 0b001, "M": 0b010, "J": 0b011, "S": 0b100, "N": 0b101}

RI_CODE = {"NOT": 0b00000, "AND": 0b00001, "OR": 0b00010, "XOR": 0b00011,
           "STL": 0b00100, "STR": 0b00101, "RTL": 0b00110, "RTR": 0b00111,
           "ADD": 0b01000, "SUB": 0b01001, "NEG": 0b01011, "MUL": 0b01100,
           "DIV": 0b01110, "MOD": 0b01111, "SET": 0b10000, "CMP": 0b10001}
M_CODE = {"LOAD": 0b00000, "SEND": 0b00001}
S_CODE = {"PUSH": 0b00000, "POP": 0b00001, "PRINT": 0b10000, "PRINTC": 0b10001}
J_CODE = {"WEQ": 0b00000, "WNE": 0b00001, "WGT": 0b00010, "WLT": 0b00011, "WCY": 0b00100,
          "WOV": 0b00101, "WDZ": 0b00110, "GOTO": 0b01000, "CAL": 0b01001}
N_CODE = {"RET": 0b00000, "END": 0b11111, "CLR": 0b10000}

# ---- README.md: operand shapes ----------------------------------------------------------------
THREE_OPERAND = {"AND", "OR", "XOR", "STL", "STR", "RTL", "RTR", "ADD", "SUB", "MUL", "DIV", "MOD"}
TWO_OPERAND = {"SET", "NOT", "NEG", "CMP"}          # Rdst [R | IMM]; CMP has no destination
OPCODES = set(RI_CODE) | set(M_CODE) | set(S_CODE) | set(J_CODE) | set(N_CODE)

REG_MAX = 15            # R0 .. R15
IMM_MAX = 0xFFFF        # #0 .. #65535
ADDR_MAX = 0xFFFF
VAR_BASE = 0x0100       # first automatically allocated variable
VAR_STEP = 2            # one 16-bit word

_RE_REG = re.compile(r"R([0-9]+)\Z")
_RE_IMM = re.compile(r"#([0-9]+)\Z")
_RE_ADDR = re.compile(r"0x([0-9A-Fa-f]+)\Z")
_RE_LABEL = re.compile(r"@([A-Za-z_][A-Za-z0-9_]*)\Z")
_RE_VAR = re.compile(r"[A-Za-z][A-Za-z0-9_]*\Z")


def _tokens(line_text):
    """(column, text) pairs. Columns are 1-based byte offsets, a tab counts as one."""
    text = line_text.split(";", 1)[0]
    return [(m.start() + 1, m.group()) for m in re.finditer(r"[^ \t\r\n]+", text)]


def _classify(tok, line, col):
    if tok in OPCODES:
        return ("op", tok)
    m = _RE_REG.match(tok)
    if m:
        n = int(m.group(1))
        if n > REG_MAX:
            raise RefError(line, col, "register out of range")
        return ("reg", n)
    m = _RE_IMM.match(tok)
    if m:
        n = int(m.group(1))
        if n > IMM_MAX:
            raise RefError(line, col, "immediate out of range")
        return ("imm", n)
    m = _RE_ADDR.match(tok)
    if m:
        n = int(m.group(1), 16)
        if n > ADDR_MAX:
            raise RefError(line, col, "address out of range")
        return ("addr", n)
    m = _RE_LABEL.match(tok)
    if m:
        return ("label", m.group(1))
    if _RE_VAR.match(tok):
        return ("var", tok)          # includes a lone "R": any name that is not an opcode or register
    raise RefError(line, col, f"cannot parse '{tok}'")


def _encode(kind, code, dest=0, src1=0, src2=0, imm=0, addr=0):
    t = TYPE[kind]
    op = (t << 5) | code
    if kind == "R":
        return (op << 24) | (dest << 20) | (src1 << 16) | (src2 << 12)
    if kind == "I":
        return (op << 24) | (dest << 20) | (src1 << 16) | imm
    if kind == "M":
        return (op << 24) | (dest << 20) | addr
    if kind == "J":
        return (op << 24) | addr
    if kind == "S":
        return (op << 24) | (dest << 20)
    return op << 24                                    # N


def assemble(source):
    # ---- pass 1: split into instructions, assign label values ----
    insts = []                 # (line, [(col, kind, value)...]) with kind of the opcode first
    labels = {}
    for ln, text in enumerate(source.split("\n"), 1):
        toks = _tokens(text)
        if not toks:
            continue
        col0, first = toks[0]
        if first.startswith("@"):
            m = _RE_LABEL.match(first)
            if not m:
                raise RefError(ln, col0, "bad label")
            if len(toks) > 1:
                raise RefError(ln, toks[1][0], "a label stands alone on its line")
            if m.group(1) in labels:
                raise RefError(ln, col0, "label defined twice")
            labels[m.group(1)] = 4 * len(insts)         # a label takes no space
            continue
        if first not in OPCODES:
            raise RefError(ln, col0, "line must start with an opcode or a label")
        classified = [(col0, "op", first)]
        for col, tok in toks[1:]:
            kind, value = _classify(tok, ln, col)
            classified.append((col, kind, value))
        insts.append((ln, classified))

    # ---- pass 2: validate operand shapes and encode ----
    variables = {}

    def variable(name):
        if name not in variables:
            variables[name] = VAR_BASE + VAR_STEP * len(variables)
        return variables[name]

    words = []
    for ln, items in insts:
        _, _, op = items[0]
        ops = items[1:]
        kinds = [k for _, k, _ in ops]
        vals = [v for _, _, v in ops]

        def bad(msg="operands do not match the instruction"):
            raise RefError(ln, items[0][0], msg)

        if op in THREE_OPERAND:
            if len(ops) != 3 or kinds[:2] != ["reg", "reg"] or kinds[2] not in ("reg", "imm"):
                bad()
            if kinds[2] == "reg":
                words.append(_encode("R", RI_CODE[op], vals[0], vals[1], vals[2]))
            else:
                words.append(_encode("I", RI_CODE[op], vals[0], vals[1], imm=vals[2]))
        elif op in TWO_OPERAND:
            if len(ops) != 2 or kinds[0] != "reg" or kinds[1] not in ("reg", "imm"):
                bad()
            if kinds[1] == "reg":            # first operand in the dest field, src1 = 0, second in src2
                words.append(_encode("R", RI_CODE[op], vals[0], 0, vals[1]))
            else:
                words.append(_encode("I", RI_CODE[op], vals[0], 0, imm=vals[1]))
        elif op in M_CODE:
            if len(ops) != 2 or kinds[0] != "reg" or kinds[1] not in ("addr", "var"):
                bad()
            addr = vals[1] if kinds[1] == "addr" else variable(vals[1])
            words.append(_encode("M", M_CODE[op], vals[0], addr=addr))
        elif op in J_CODE:
            if len(ops) != 1 or kinds[0] != "label":
                bad()
            if vals[0] not in labels:
                raise RefError(ln, ops[0][0], "undefined label")
            words.append(_encode("J", J_CODE[op], addr=labels[vals[0]]))
        elif op in S_CODE:
            if len(ops) != 1 or kinds[0] != "reg":
                bad()
            words.append(_encode("S", S_CODE[op], vals[0]))
        else:                                # N-type
            if ops:
                bad()
            words.append(_encode("N", N_CODE[op]))

    return MAGIC + b"".join(w.to_bytes(4, "big") for w in words)


if __name__ == "__main__":
    import sys
    try:
        sys.stdout.write(assemble(open(sys.argv[1]).read()).hex() + "\n")
    except RefError as e:
        sys.exit(f"error {e}")
