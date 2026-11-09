#pragma once

#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>
#include <variant>
#include <cstdint>

namespace instruction_mod {

    // ----------------------------------------------------------------
    inline constexpr size_t INST_SIZE = 4; //tokens in an instruction: the opcode plus up to three operands
    inline constexpr size_t MAX_REG_COUNT = 16;
    inline constexpr int MAX_ADDR_VAL = 65535;
    inline constexpr int MAX_IMM_VAL = 65535;
    inline constexpr size_t ADR_OFFSET = 0x0100; //preallocated memory
    inline constexpr size_t INST_BYTES = 4; //an encoded instruction is 32 bits, label addresses are counted in bytes
    inline constexpr size_t VAR_BYTES = 2; //a variable is one 16 bit word
    // ----------------------------------------------------------------

    enum class OpCode {

        //  memory and registers
        LOAD,
        SEND,
        SET,

        //  bitwise
        NOT,
        AND,
        OR,
        XOR,

        //  shift and rotation
        STL,
        STR,
        RTL,
        RTR,

        //  arithmetic
        NEG,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,

        //  comparison and branching
        CMP,
        GOTO,
        WEQ,
        WNE,
        WGT,
        WLT,
        WCY,
        WOV,
        WDZ,

        //  stacks and sub-routines
        CAL,
        RET,
        PUSH,
        POP,

        //  i/o
        PRINT,
        PRINTC,

        //  misc. and debugging
        END,
        CLR,

    };

    enum class InstType {
        R,
        I,
        M,
        S,
        J,
        N,
        LBL, //labels skipped by codegen
        INV, //invalid
    };

    struct OpCodeToken    { OpCode value; };
    struct RegisterToken  { int value; };
    struct ImmediateToken { int value; };
    struct AddressToken   { int value; };
    struct LabelToken     { std::string name; };
    struct VariableToken  { std::string name; };
    struct InvalidToken   { }; //a number that could not be read, shouldn't happen ideally

    using TokenKind = std::variant<OpCodeToken, RegisterToken, ImmediateToken, AddressToken, LabelToken, VariableToken, InvalidToken>;

    class Token {

        public:

            TokenKind kind;
            size_t file_line;
            size_t file_col;
            size_t file_end_col; //column just after the token, where the lexer noticed that it ended

            Token();
            Token(TokenKind kind, size_t f_line, size_t f_col, size_t f_end_col);

    };

    std::string_view token_type_name(const Token& token); //"OpCode", "Register", ... for debug output

    using OperandMask = unsigned;
    inline constexpr OperandMask OPR_NONE = 0; //no operand in this position
    inline constexpr OperandMask OPR_REG = 1 << 0;
    inline constexpr OperandMask OPR_IMM = 1 << 1;
    inline constexpr OperandMask OPR_ADR = 1 << 2;
    inline constexpr OperandMask OPR_VAR = 1 << 3;
    inline constexpr OperandMask OPR_LBL = 1 << 4;

    OperandMask operand_kind(const Token& token); //which OPR_ bit a token is, OPR_NONE for opcodes and invalid tokens

    using OperandFmt = std::array<OperandMask, INST_SIZE - 1>;

    struct InstInfo {
        std::string_view mnemonic;
        InstType type;                      //the form of the instruction. For an R/I pair this is the register form (R)
        std::optional<InstType> imm_type;   //the immediate form (I) of an R/I pair, empty for every other instruction
        std::uint8_t code;                  //the 5 bit instruction code
        OperandFmt fmt;                     //which operand kinds are accepted in each operand position
    };

    extern const std::unordered_map<OpCode, InstInfo> instruction_table; //defined in instructions.cpp
    const InstInfo& instruction_info(OpCode opcode); //panics if the table has no entry, which would be a bug
    std::optional<OpCode> find_opcode(std::string_view mnemonic); //mnemonic -> opcode, empty if it is not an instruction

    class Inst {

        public:

            size_t used_size;
            InstType inst_type;
            std::optional<size_t> inst_no;
            std::array<std::optional<Token>, INST_SIZE> token_arr; // INST_SIZE is a namespace constant

            Inst();
            bool push_token(Token&& token);

    };

    using Pipeline = std::vector<Inst>;

}
