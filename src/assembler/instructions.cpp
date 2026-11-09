#include "instructions.hpp"
#include "../overload.hpp"
#include "../panic.hpp"

namespace instruction_mod {

    // single instruction table: add or change an instruction here and nowhere else
    namespace {
        constexpr OperandFmt FMT_NULL{ OPR_NONE, OPR_NONE, OPR_NONE };                                        // N
        constexpr OperandFmt FMT_REG{ OPR_REG, OPR_NONE, OPR_NONE };                                          // S
        constexpr OperandFmt FMT_REG_ADR{ OPR_REG, OPR_ADR | OPR_VAR, OPR_NONE };                             // M
        constexpr OperandFmt FMT_REG_RGI{ OPR_REG, OPR_REG | OPR_IMM, OPR_NONE };                             // R/I with two operands
        constexpr OperandFmt FMT_REG_REG_RGI{ OPR_REG, OPR_REG, OPR_REG | OPR_IMM };                          // R/I with three operands
        constexpr OperandFmt FMT_LBL{ OPR_LBL, OPR_NONE, OPR_NONE };                                          // J
    }

    const std::unordered_map<OpCode, InstInfo> instruction_table {

        // memory and registers
        { OpCode::LOAD,   { "LOAD",   InstType::M, std::nullopt,   0b00000, FMT_REG_ADR } },
        { OpCode::SEND,   { "SEND",   InstType::M, std::nullopt,   0b00001, FMT_REG_ADR } },
        { OpCode::SET,    { "SET",    InstType::R, InstType::I,    0b10000, FMT_REG_RGI } },

        // bitwise
        { OpCode::NOT,    { "NOT",    InstType::R, InstType::I,    0b00000, FMT_REG_RGI } },
        { OpCode::AND,    { "AND",    InstType::R, InstType::I,    0b00001, FMT_REG_REG_RGI } },
        { OpCode::OR,     { "OR",     InstType::R, InstType::I,    0b00010, FMT_REG_REG_RGI } },
        { OpCode::XOR,    { "XOR",    InstType::R, InstType::I,    0b00011, FMT_REG_REG_RGI } },

        // shift and rotation
        { OpCode::STL,    { "STL",    InstType::R, InstType::I,    0b00100, FMT_REG_REG_RGI } },
        { OpCode::STR,    { "STR",    InstType::R, InstType::I,    0b00101, FMT_REG_REG_RGI } },
        { OpCode::RTL,    { "RTL",    InstType::R, InstType::I,    0b00110, FMT_REG_REG_RGI } },
        { OpCode::RTR,    { "RTR",    InstType::R, InstType::I,    0b00111, FMT_REG_REG_RGI } },

        // arithmetic
        { OpCode::NEG,    { "NEG",    InstType::R, InstType::I,    0b01011, FMT_REG_RGI } },
        { OpCode::ADD,    { "ADD",    InstType::R, InstType::I,    0b01000, FMT_REG_REG_RGI } },
        { OpCode::SUB,    { "SUB",    InstType::R, InstType::I,    0b01001, FMT_REG_REG_RGI } },
        { OpCode::MUL,    { "MUL",    InstType::R, InstType::I,    0b01100, FMT_REG_REG_RGI } },
        { OpCode::DIV,    { "DIV",    InstType::R, InstType::I,    0b01110, FMT_REG_REG_RGI } },
        { OpCode::MOD,    { "MOD",    InstType::R, InstType::I,    0b01111, FMT_REG_REG_RGI } },

        // comparison and branching
        { OpCode::CMP,    { "CMP",    InstType::R, InstType::I,    0b10001, FMT_REG_RGI } },
        { OpCode::GOTO,   { "GOTO",   InstType::J, std::nullopt,   0b01000, FMT_LBL } },
        { OpCode::WEQ,    { "WEQ",    InstType::J, std::nullopt,   0b00000, FMT_LBL } },
        { OpCode::WNE,    { "WNE",    InstType::J, std::nullopt,   0b00001, FMT_LBL } },
        { OpCode::WGT,    { "WGT",    InstType::J, std::nullopt,   0b00010, FMT_LBL } },
        { OpCode::WLT,    { "WLT",    InstType::J, std::nullopt,   0b00011, FMT_LBL } },
        { OpCode::WCY,    { "WCY",    InstType::J, std::nullopt,   0b00100, FMT_LBL } },
        { OpCode::WOV,    { "WOV",    InstType::J, std::nullopt,   0b00101, FMT_LBL } },
        { OpCode::WDZ,    { "WDZ",    InstType::J, std::nullopt,   0b00110, FMT_LBL } },

        // stacks and sub-routines
        { OpCode::CAL,    { "CAL",    InstType::J, std::nullopt,   0b01001, FMT_LBL } },
        { OpCode::RET,    { "RET",    InstType::N, std::nullopt,   0b00000, FMT_NULL } },
        { OpCode::PUSH,   { "PUSH",   InstType::S, std::nullopt,   0b00000, FMT_REG } },
        { OpCode::POP,    { "POP",    InstType::S, std::nullopt,   0b00001, FMT_REG } },

        // i/o (VM only)
        { OpCode::PRINT,  { "PRINT",  InstType::S, std::nullopt,   0b10000, FMT_REG } },
        { OpCode::PRINTC, { "PRINTC", InstType::S, std::nullopt,   0b10001, FMT_REG } },

        // misc. and debugging
        { OpCode::END,    { "END",    InstType::N, std::nullopt,   0b11111, FMT_NULL } },
        { OpCode::CLR,    { "CLR",    InstType::N, std::nullopt,   0b10000, FMT_NULL } },

    };

    const InstInfo& instruction_info(OpCode opcode) {
        auto it = instruction_table.find(opcode);
        if (it == instruction_table.end()) panic::panic("instruction_info: opcode missing from the instruction table");
        return it->second;
    }

    std::optional<OpCode> find_opcode(std::string_view mnemonic) {
        for (const auto& [opcode, info] : instruction_table) {
            if (info.mnemonic == mnemonic) return opcode;
        }
        return std::nullopt;
    }

    // tokens: constructors and helpers that say what a token is; replaced TokenType field and TokenOpt confusing ahh shi
    Token::Token() :
        kind{InvalidToken{}}, file_line{0}, file_col{0}, file_end_col{0} { }
    Token::Token(TokenKind kind, size_t f_line, size_t f_col, size_t f_end_col) :
        kind{std::move(kind)}, file_line{f_line}, file_col{f_col}, file_end_col{f_end_col} { }

    std::string_view token_type_name(const Token& token) {
        return std::visit(overload{
            [](const OpCodeToken&)    -> std::string_view { return "OpCode"; },
            [](const RegisterToken&)  -> std::string_view { return "Register"; },
            [](const ImmediateToken&) -> std::string_view { return "Immediate"; },
            [](const AddressToken&)   -> std::string_view { return "Address"; },
            [](const LabelToken&)     -> std::string_view { return "Label"; },
            [](const VariableToken&)  -> std::string_view { return "Variable"; },
            [](const InvalidToken&)   -> std::string_view { return "Invalid"; },
        }, token.kind);
    }

    OperandMask operand_kind(const Token& token) {
        return std::visit(overload{
            [](const RegisterToken&)  { return OPR_REG; },
            [](const ImmediateToken&) { return OPR_IMM; },
            [](const AddressToken&)   { return OPR_ADR; },
            [](const LabelToken&)     { return OPR_LBL; },
            [](const VariableToken&)  { return OPR_VAR; },
            [](const auto&)           { return OPR_NONE; }, //opcodes and invalid tokens are not operands
        }, token.kind);
    }

    Inst::Inst() :
        used_size{0},
        inst_type{InstType::INV},
        inst_no{std::nullopt},
        token_arr{std::nullopt}
    { }

    bool Inst::push_token(Token&& token) {
        if (used_size < INST_SIZE) {
            token_arr[used_size] = std::move(token);
            ++used_size;
            return true;
        }
        return false;
    }

}