#include "layout.hpp"

namespace layout_mod {

    void Layout::run(const instruction_mod::Pipeline& pipeline, symbol_mod::SymbolTable& symbols) const {

        for (const auto& name : symbols.label_names()) {
            symbols.set_label_address(name, symbols.label_inst_index(name) * instruction_mod::INST_BYTES);
        }

        size_t next_unused_adr{instruction_mod::ADR_OFFSET};
        for (const auto& inst : pipeline) {
            if (inst.inst_type != instruction_mod::InstType::M) continue;
            const auto& adr_token = inst.token_arr[2];
            if (!adr_token.has_value()) continue;
            if (const auto* var = std::get_if<instruction_mod::VariableToken>(&adr_token->kind)) {
                if (!symbols.variable_address(var->name)) {
                    symbols.set_variable_address(var->name, next_unused_adr);
                    next_unused_adr += instruction_mod::VAR_BYTES;
                }
            }
        }

    }

}
