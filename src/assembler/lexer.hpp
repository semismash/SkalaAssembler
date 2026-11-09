#pragma once

#include <cctype>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <charconv>
#include <expected>
#include <optional>

#include "instructions.hpp"
#include "error.hpp"

namespace lexer_mod {

    using error::Error;
    using LexErr = error::LexError;

    enum class State {
        Idn,    //identifier mode, transition state, can be pushed as variable or opcode
        Rgt,    //register transition state (when 'R' is detected)
        Reg,    //register
        Sep,    //seperator (,)
        Imt,    //immediate transition state (when '#' is detected)
        Imm,    //immediate
        Lbt,    //label transition (when '@' is detected)
        Lbl,    //label
        Zer,    //zero prefix, transition state
        Adt,    //address transition ('x' in '0x')
        Adr,    //address (eg. 0xDEADBEEF)
        Cmt,    //comment (;)
        Nil,    //none (whitespace type char)
        Err,    //error/unknown state
    };

    enum class StateType {
        Word,
        RTransition, // special variation of transition since its alphanumeric
        ZTransition, // special variation for 0 prefix as there are two characters required for the address prefix
        Transition,
        None,
    };

    extern const std::unordered_map<State, StateType> state_map; //defined in parser.cpp

    enum class Action {
        Push,   //push character to buffer
        Emit,   //emit token in buffer
        Idle,   //do nothing
    };

    // The result of lexing one line: its tokens, and the lexical error that ended the line early (if any).
    struct LexResult {
        std::vector<instruction_mod::Token> tokens;
        std::optional<Error> error;
    };

    // Turns the text of one line into tokens. The state machine is the one that used to live in the parser.
    class Lexer {

        private:
            State cur_state;
            State prev_state;
            Action cur_action;
            std::string buffer;
            unsigned char cur_ch;
            size_t line_count;
            size_t col_count;
            size_t tkn_start_char_col;

            std::expected<void, Error> set_state();
            std::expected<void, Error> set_action();
            std::expected<void, Error> execute(std::vector<instruction_mod::Token>& tokens);
            std::unexpected<Error> lex_error(LexErr err);

        public:

            Lexer();
            // The line must not contain the '\n' that ends it. line_no is only used for positions.
            LexResult lex_line(std::string_view line, size_t line_no);

    };

}
