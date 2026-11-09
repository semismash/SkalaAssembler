#include "classifier.hpp"
#include "../overload.hpp"

namespace classifier_mod {

    Classifier::Classifier() :
        inst_count{0}
    { }

    std::expected<void, Error> Classifier::classify(instruction_mod::Inst& inst, symbol_mod::SymbolTable& symbols) {

        const auto& first_token = inst.token_arr[0];
        if (!first_token.has_value()) { //should not happen, the parser never creates an empty instruction
            return std::unexpected(Error(ClassifyErr::MissingOpCodeError, 0, 0));
        }

        return std::visit(overload{

            [this, &inst](const instruction_mod::OpCodeToken& opcode) -> std::expected<void, Error> {
                inst.inst_no = inst_count++;
                const auto& info = instruction_mod::instruction_info(opcode.value);
                inst.inst_type = info.type;
                //an instruction that has both a register and an immediate form is an I-type if its last operand is an immediate
                if (info.imm_type.has_value() && inst.used_size > 1) {
                    const auto& last_token = inst.token_arr[inst.used_size - 1];
                    if (last_token.has_value() && std::holds_alternative<instruction_mod::ImmediateToken>(last_token->kind)) {
                        inst.inst_type = *info.imm_type;
                    }
                }
                return {};
            },

            [this, &inst, &symbols, &first_token](const instruction_mod::LabelToken& label) -> std::expected<void, Error> {
                if (inst.used_size > 1) { //extra operands after the label
                    return std::unexpected(Error(ClassifyErr::LabelTooLongError, first_token->file_line, first_token->file_col));
                }
                if (!symbols.declare_label(label.name, inst_count)) {
                    return std::unexpected(Error(ClassifyErr::LabelAlreadyExists, first_token->file_line, first_token->file_col));
                }
                inst.inst_type = instruction_mod::InstType::LBL;
                return {};
            },

            [](const auto&) -> std::expected<void, Error> { return {}; } //anything else is reported by the analyzer

        }, first_token->kind);

    }

}
