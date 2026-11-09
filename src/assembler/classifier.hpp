#pragma once

#include <expected>

#include "instructions.hpp"
#include "symbol_table.hpp"
#include "error.hpp"

namespace classifier_mod {

    using error::Error;
    using ClassifyErr = error::ClassifyError;

    // Mechanical work that has to happen before analysis: numbering instructions, declaring labels
    // in the symbol table and working out the form (R or I, M, J, ...) of every instruction.
    // It does not judge whether a program is valid, that is the analyzer's job.
    class Classifier {

        private:
            size_t inst_count;

        public:
            Classifier();
            std::expected<void, Error> classify(instruction_mod::Inst& inst, symbol_mod::SymbolTable& symbols);

    };

}
