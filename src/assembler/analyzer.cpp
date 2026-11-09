#include "analyzer.hpp"
#include "../overload.hpp"
#include "../panic.hpp"

namespace analyzer_mod {

    //validate specific tokens to see if they are within constraints, only checked for positions 2 to INSTSIZE
    std::expected<void, Error> Analyzer::validate_token(const instruction_mod::Token& token, const symbol_mod::SymbolTable& symbols) const {
        using namespace instruction_mod;
        return std::visit(overload{
            [&token](const OpCodeToken&) -> std::expected<void, Error> { //shouldn't happen ideally
                return std::unexpected(Error(SemErr::InvalidOpCodePosition, token.file_line, token.file_col));
            },
            [&token](const RegisterToken& reg) -> std::expected<void, Error> {
                if (reg.value < 0 || reg.value >= static_cast<int>(MAX_REG_COUNT)) {
                    return std::unexpected(Error(SemErr::RegisterOutOfRange, token.file_line, token.file_col));
                }
                return {};
            },
            [&token](const ImmediateToken& imm) -> std::expected<void, Error> {
                if (imm.value < 0 || imm.value > MAX_IMM_VAL) {
                    return std::unexpected(Error(SemErr::ImmediateOutOfRange, token.file_line, token.file_col));
                }
                return {};
            },
            [&token](const AddressToken& adr) -> std::expected<void, Error> {
                if (adr.value < 0 || adr.value > MAX_ADDR_VAL) {
                    return std::unexpected(Error(SemErr::AddressOutOfRange, token.file_line, token.file_col));
                }
                return {};
            },
            [&token, &symbols](const LabelToken& label) -> std::expected<void, Error> {
                if (symbols.has_label(label.name)) {
                    return {};
                }
                return std::unexpected(Error(SemErr::UndefinedLabelError, token.file_line, token.file_col));
            },
            [](const VariableToken&) -> std::expected<void, Error> {
                return {}; //no further verification required
            },
            [&token](const InvalidToken&) -> std::expected<void, Error> { //never reaches here, an invalid token is rejected before it is validated
                return std::unexpected(Error(SemErr::IncorrectOperandFmt, token.file_line, token.file_col));
            },
        }, token.kind);
    }

    //checks the operands of an instruction against what its opcode accepts
    std::expected<void, Error> Analyzer::validate_opcode(const instruction_mod::Inst& inst, instruction_mod::OpCode opcode, const symbol_mod::SymbolTable& symbols) const {

        const auto& info = instruction_mod::instruction_info(opcode);

        for (size_t tkn{1}; tkn < instruction_mod::INST_SIZE; ++tkn) {

            const auto& cur_token = inst.token_arr[tkn];
            const instruction_mod::OperandMask cur_target = info.fmt[tkn-1];

            if (cur_token.has_value() != (cur_target != instruction_mod::OPR_NONE)) { // one is missing and the other is not
                const auto& at = cur_token.has_value() ? cur_token.value() : inst.token_arr[0].value();
                return std::unexpected(Error(SemErr::IncorrectOperandFmt, at.file_line, at.file_col));
            } else if (cur_token.has_value()) {
                if ((cur_target & instruction_mod::operand_kind(*cur_token)) == 0) {
                    return std::unexpected(Error(SemErr::IncorrectOperandFmt, cur_token->file_line, cur_token->file_col));
                } else {
                    const auto tkn_val_res = validate_token(cur_token.value(), symbols);
                    if (!tkn_val_res) return tkn_val_res;
                }
            }

        }

        return {};

    }

    //analyzer
    std::expected<void, Error> Analyzer::analyze(const instruction_mod::Inst& inst, const symbol_mod::SymbolTable& symbols) const {
        const auto& first_token = inst.token_arr[0];
        if (!first_token.has_value()) { //check if first token exists, should not happen ideally but still
            return std::unexpected(Error(SemErr::MissingOpCodeError, 0, 0));
        }
        return std::visit(overload{
            [this, &inst, &symbols](const instruction_mod::OpCodeToken& opcode) -> std::expected<void, Error> {
                return validate_opcode(inst, opcode.value, symbols);
            },
            [](const instruction_mod::LabelToken&) -> std::expected<void, Error> {
                return {}; //intentionally empty, labels are handled by the classifier
            },
            [&first_token](const auto&) -> std::expected<void, Error> {
                return std::unexpected(Error(SemErr::IncorrectFirstToken, first_token->file_line, first_token->file_col));
            },
        }, first_token->kind);
    }

}