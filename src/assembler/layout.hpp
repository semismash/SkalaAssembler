#pragma once

#include "instructions.hpp"
#include "symbol_table.hpp"

namespace layout_mod {

    // decides where things go: the byte address of every label, and the address of every variable.
    class Layout {

        public:

            // Labels get (number of instructions before them) * INST_BYTES; variables are allcoated
            // from ADR_OFFSET upwards, in the order of their first use in the program
            void run(const instruction_mod::Pipeline& pipeline, symbol_mod::SymbolTable& symbols) const;

    };

}
