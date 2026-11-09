#pragma once

#include <vector>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <magic_enum/magic_enum.hpp> //debugging

#include "instructions.hpp"
#include "error.hpp"
#include "symbol_table.hpp"
#include "parser.hpp"
#include "classifier.hpp"
#include "analyzer.hpp"
#include "layout.hpp"
#include "codegen.hpp"

namespace fs = std::filesystem;

namespace assembler_mod {
    
    inline constexpr std::string_view prog_name = "demo";
    inline constexpr uint32_t MAGIC_NUM = 0x3cDEC0DE;

    class Assembler {

        private:
            error::ErrorLog error_log;
            fs::path output_path;
            bool output_error{false};

            // the stages of assemble_prog, one function each
            void parse_source(instruction_mod::Pipeline& pipeline, const std::string& file);
            void classify(instruction_mod::Pipeline& pipeline);
            void analyze(const instruction_mod::Pipeline& pipeline);
            void generate(const instruction_mod::Pipeline& pipeline);

        public:
            symbol_mod::SymbolTable symbols;

            Assembler();
            bool has_errors() const { return error_log.has_errors(); } // lets main return an exit code
            bool has_output_error() const { return output_error; } // lets main return exit code 2 when the output file cannot be written
            void set_output_path(const fs::path& path) { output_path = path; } // used by the -o command-line option
            void assemble_prog(instruction_mod::Pipeline& pipeline, const std::string& file);
            void dbg_display_tokens(const instruction_mod::Pipeline& pipeline) const;
            void dbg_display_labels() const;
            void display_errors();

    };

}
