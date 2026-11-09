#pragma once

#include <expected>

#include "instructions.hpp"
#include "symbol_table.hpp"
#include "error.hpp"

namespace analyzer_mod {

    using error::Error;
    using SemErr = error::SemanticError;

    class Analyzer {

        public:
            std::expected<void, Error> validate_token(const instruction_mod::Token& token, const symbol_mod::SymbolTable& symbols) const;
            std::expected<void, Error> validate_opcode(const instruction_mod::Inst& inst, instruction_mod::OpCode opcode, const symbol_mod::SymbolTable& symbols) const;
            std::expected<void, Error> analyze(const instruction_mod::Inst& inst, const symbol_mod::SymbolTable& symbols) const;

    };

}