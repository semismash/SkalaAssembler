#pragma once

#include <string>
#include <string_view>

#include "instructions.hpp"
#include "lexer.hpp" 
#include "error.hpp"

namespace parser_mod { 

    using error::Error;
    using ParseErr = error::ParseError;

    class Parser {

        private:

            lexer_mod::Lexer lexer;

            void parse_line(std::string_view line, size_t line_no, instruction_mod::Pipeline& pipeline, error::ErrorLog& error_log);

        public:

            Parser();

            // Splits the source into lines, and adds one instruction to the pipeline for every line that has tokens.
            // A line with an error adds nothing, its error is logged and the rest of that line is ignored.
            void parse_source(const std::string& source, instruction_mod::Pipeline& pipeline, error::ErrorLog& error_log);

    };

}
