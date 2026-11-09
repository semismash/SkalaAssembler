# skala-v1.1: what changed (A1 to A9)

Every change to an existing file is marked `// NEW:`. Small changes are marked on the line. Larger blocks have a
`// NEW:` line above them and dashed lines before and after. New files are not marked.

| Step | What | Where |
| --- | --- | --- |
| A1 | Command-line arguments, `panic` is `[[noreturn]]` and never waits on stdin | `main.cpp`, `panic.hpp/.cpp`, `assembler.hpp/.cpp` (output path) |
| A2 | Test suite, with a sanitizer-friendly runner | `tests/` |
| A3 | The five bug fixes (already in `skala-v1`) | `lexer.cpp` (hex base, lone `R`), `codegen.cpp`, `analyzer.cpp`, `main.cpp` |
| A4 | `Token` is a variant of small structs: no `TokenType`, no `TokenOpt`, about 30 `panic` branches gone | `instructions.hpp/.cpp`, `analyzer.cpp`, `codegen.cpp` |
| A5 | Lexer and parser split. The state machine moved unchanged to `lexer.cpp`, the parser builds instructions from tokens | `lexer.hpp/.cpp` (new), `parser.hpp/.cpp` |
| A6 | One instruction table replaces `instruction_map`, `instruction_types`, `instruction_fmt`, `opcode_mapping`. The `RI` placeholder is gone | `instructions.hpp/.cpp` |
| A7 | `Classifier`, `SymbolTable` and `Layout` are separate. The analyzer only checks, `CodeGen` has no state | `classifier.*`, `symbol_table.*`, `layout.*` (new), `analyzer.*`, `codegen.*` |
| A8 | `ParsingError` split by stage (`LexError`, `ParseError`, `ClassifyError`, `CodeGenError`), `ErrorLog` extracted from `Assembler` | `error.hpp/.cpp` |
| A9 | `assemble_prog` is a thin sequence of stages, unused member removed, ISA constants moved out of `Inst`, warnings fixed, typo fixed | `assembler.*`, `instructions.hpp` |

## Pipeline

```
source -> Lexer (per line) -> Parser -> Classifier -> Analyzer -> Layout -> CodeGen -> .bin
          tokens             Inst      labels,       checks      label and   words
                                       R/I form                  variable
                                                                 addresses
```

## How it was checked

* The suite: 152 tests in command-line mode (141 without `--cli`), including a seeded fuzzer compared with an
  independent reference encoder.
* A differential run of the A1 build against this build on 18,000 mutated and garbage programs. Exit code, full
  output and binary were identical every time. (The debug token dump was compared with the token type names
  normalized, because my copy of `magic_enum` is a stub. The new names are the same strings the old `TokenType` enum had.)
* `-Wall -Wextra`: 55 warnings before, 0 now.

## Differences you will notice

* The message "Specified Label should not have extra operards" is now "operands". The only golden snapshot that changed.
* On a program with errors, the debug dump (`dbg_display_tokens`) shows the real instruction type of instructions
  that failed analysis. They used to show `INV`, because the type was set only after a successful analysis.
* An output file that cannot be opened is an error with exit code 2 (it used to be silent). Added in A1.

## Suggested review order

1. `instructions.hpp` and `instructions.cpp` (tokens and the instruction table, A4 and A6)
2. `lexer.hpp`, `parser.cpp` (A5): the lexer is the old state machine, only its input and output changed
3. `classifier.cpp`, `analyzer.cpp`, `layout.cpp`, `symbol_table.cpp` (A7)
4. `error.hpp/.cpp`, `assembler.cpp` (A8 and A9)
