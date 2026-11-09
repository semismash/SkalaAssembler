#include "parser.hpp"

namespace parser_mod {

    Parser::Parser() :
        lexer()
    { }

    void Parser::parse_source(const std::string& source, instruction_mod::Pipeline& pipeline, error::ErrorLog& error_log) {
        const std::string_view text{source};
        size_t line_no{1};
        size_t start{0};
        while (true) {
            const size_t end = text.find('\n', start);
            if (end == std::string_view::npos) { //the last line, it does not need to end with a newline
                parse_line(text.substr(start), line_no, pipeline, error_log);
                break;
            }
            parse_line(text.substr(start, end - start), line_no, pipeline, error_log);
            start = end + 1;
            ++line_no;
        }
    }

    void Parser::parse_line(std::string_view line, size_t line_no, instruction_mod::Pipeline& pipeline, error::ErrorLog& error_log) {

        auto lexed = lexer.lex_line(line, line_no);

        instruction_mod::Inst inst;
        for (auto& token : lexed.tokens) {
            const size_t end_col = token.file_end_col; //read before the token is moved
            if (!inst.push_token(std::move(token))) {
                //a fifth token. This is reported (and wins over a lexical error later in the line), exactly like before
                error_log.log(Error(ParseErr::InstructionTooLong, line_no, end_col));
                return;
            }
        }

        if (lexed.error.has_value()) { //the tokens before a lexical error are thrown away
            error_log.log(*lexed.error);
            return;
        }

        if (inst.used_size != 0) {
            pipeline.push_back(std::move(inst));
        }

    }

}