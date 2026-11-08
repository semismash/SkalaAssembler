> **Status:** There is currently no VM or hardware that runs Skala binaries. This document is the reference for the instruction set and for the binary the assembler produces. PRINT and PRINTC are debugging instructions intended for a future VM. The assembler encodes them, but nothing executes them yet.

# Instruction Types

## R-instruction
- Register only  
- OPCODE REG REG (REG)  
- SET, NOT, AND, OR, XOR, STL, STR, RTL, RTR, NEG, ADD, SUB, MUL, DIV, MOD, CMP  

## I-instruction
- Register + Immediate  
- OPCODE REG (REG) IMM  
- SET, NOT, AND, OR, XOR, STL, STR, RTL, RTR, NEG, ADD, SUB, MUL, DIV, MOD, CMP

## M-instruction
- Memory Management  
- OPCODE REG ADR
- LOAD, SEND  

## J-instruction
- Jumping and Branching  
- OPCODE @LABEL  
- WEQ, WNE, WGT, WLT, WCY, WOV, WDZ, GOTO, CAL  

## S-instruction
- Single Register Operation  
- OPCODE REG  
- PUSH, POP, PRINT (debug, needs a VM), PRINTC (debug, needs a VM)  

## N-instruction
- No Operands  
- OPCODE  
- RET, END, CLR  

# Instruction Binary Format

Instruction Length - 32 bits (4 bytes); Word Length - 16 bits (2 bytes)
Opcode - 3 bits for type + 5 bits for instruction (8 bits total)
- R-type: (opcode 8) (dest 4 | src1 4)   (src2 4 | unused 4) (unused 8)  
- I-type: (opcode 8) (dest 4 | src  4)   (imm high 8)        (imm low 8)  
- M-type: (opcode 8) (reg  4 | unused 4) (addr high 8)       (addr low 8)  
- J-type: (opcode 8) (unused 8)          (addr high 8)       (addr low 8)
- S-type: (opcode 8) (reg  4 | unused 4) (unused 8)          (unused 8)  
- N-type: (opcode 8) (unused 8)          (unused 8)          (unused 8)  

## Operand placement
- R-type with three operands (`ADD Rdst Rax Rbx`): dest, src1, src2 in the fields above  
- R-type with two operands (`SET`, `NOT`, `NEG`, `CMP`): the first operand goes in the dest field, src1 is 0, and the second operand goes in the src2 field  
- I-type with three operands (`ADD Rdst Rax IMM`): dest, src, imm  
- I-type with two operands (`SET Rdst IMM`, `CMP Rax IMM`): the first operand goes in the dest field, src is 0, and the immediate fills the last 16 bits  
- `CMP` has no destination, so its first operand (Rax) is stored in the dest field  

## Addresses and Binary File
- A label does not take up space. Its value is the byte offset of the next instruction, which is 4 x (number of instructions before it)  
- Variables are allocated automatically on first use, starting at 0x0100 in steps of 2 bytes  
- The binary file starts with the 4-byte magic number 0x3CDEC0DE, followed by every instruction as 4 bytes, all big-endian  

# Instruction Opcodes

## Memory and Registers
LOAD Rdst ADRsrc  
SEND Rsrc ADRdst  
SET  Rdst Rsrc  

## Bitwise
NOT  Rdst Rax  
AND  Rdst Rax Rbx  
OR   Rdst Rax Rbx  
XOR  Rdst Rax Rbx  

## Shift and Rotation
STL  Rdst Rax Rbx   
STR  Rdst Rax Rbx  
RTL  Rdst Rax Rbx  
RTR  Rdst Rax Rbx  

## Arithmetic
NEG  Rdst Rax  
ADD  Rdst Rax Rbx  
SUB  Rdst Rax Rbx  
MUL  Rdst Rax Rbx  
DIV  Rdst Rax Rbx  
MOD  Rdst Rax Rbx  

## Comparison and Branching
CMP  Rax Rbx  
GOTO @LBL  
WEQ  @LBL  
WNE  @LBL  
WGT  @LBL  
WLT  @LBL  
WCY  @LBL  
WOV  @LBL  
WDZ  @LBL  

## Stacks and Sub-routines
CAL  @LBL  
RET  
PUSH Rsrc
POP  Rdst

## Misc. and Debugging
PRINT  Rsrc
PRINTC Rsrc
END  
CLR  

# OpCode Binary Codes

## Instruction Types - (first 3 bits)
- R - 000  
- I - 001  
- M - 010  
- J - 011  
- S - 100  
- N - 101  

## OpCodes - (3 bits from instruction type + 5 bits for instruction)

### R-types -
- SET - 000 10000  
- NOT - 000 00000  
- AND - 000 00001  
- OR  - 000 00010  
- XOR - 000 00011  
- STL - 000 00100  
- STR - 000 00101  
- RTL - 000 00110  
- RTR - 000 00111  
- ADD - 000 01000  
- SUB - 000 01001  
- NEG - 000 01011  
- MUL - 000 01100  
- DIV - 000 01110  
- MOD - 000 01111  
- CMP - 000 10001  

### I-types -
- SET - 001 10000  
- NOT - 001 00000  
- AND - 001 00001  
- OR  - 001 00010  
- XOR - 001 00011  
- STL - 001 00100  
- STR - 001 00101  
- RTL - 001 00110  
- RTR - 001 00111  
- ADD - 001 01000  
- SUB - 001 01001  
- NEG - 001 01011  
- MUL - 001 01100  
- DIV - 001 01110  
- MOD - 001 01111  
- CMP - 001 10001  

### M-types -
- LOAD - 010 00000  
- SEND - 010 00001  

### S-types -
- PUSH - 100 00000  
- POP  - 100 00001  
- PRINT - 100 10000  
- PRINTC - 100 10001  

## J-instruction
- WEQ  - 011 00000  
- WNE  - 011 00001  
- WGT  - 011 00010  
- WLT  - 011 00011  
- WCY  - 011 00100  
- WOV  - 011 00101  
- WDZ  - 011 00110  
- GOTO - 011 01000  
- CAL  - 011 01001  

## N-instruction
- RET - 101 00000  
- END - 101 11111  
- CLR - 101 10000