#include "lexer.hpp"
#include "../overload.hpp"
#include "../panic.hpp"

namespace lexer_mod {

    const std::unordered_map<State, StateType> state_map {
        { State::Idn, StateType::Word },
        { State::Reg, StateType::Word },
        { State::Imm, StateType::Word },
        { State::Lbl, StateType::Word },
        { State::Adr, StateType::Word },
        { State::Rgt, StateType::RTransition },
        { State::Imt, StateType::Transition },
        { State::Lbt, StateType::Transition },
        { State::Adt, StateType::Transition },
        { State::Zer, StateType::ZTransition },
        { State::Cmt, StateType::None },
        { State::Nil, StateType::None },
        { State::Err, StateType::None },
    };

    Lexer::Lexer() :
        cur_state{State::Nil},
        prev_state{State::Nil},
        cur_action{Action::Idle},
        buffer{""},
        cur_ch{0},
        line_count{1},
        col_count{1},
        tkn_start_char_col{0}
    { }

    std::expected<void, Error> Lexer::set_state() {


        if (cur_ch == ';') {

            cur_state = State::Cmt;
            
        } else if (cur_state == State::Zer && cur_ch == 'x') { //0x detection

            cur_state = State::Adt;

        } else if (std::isspace(cur_ch)) { //simple state machine lmao

            switch (cur_state) {
                case State::Idn:
                case State::Reg:
                case State::Rgt: // despite being a transition state, this is allowed since R is alphanumeric
                case State::Imm:
                case State::Lbl:
                case State::Adr:
                case State::Nil:
                    cur_state = State::Nil;
                    break;
                case State::Imt:
                case State::Zer:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbt:
                    return lex_error(LexErr::LabelNamingError);
                    break;
                case State::Adt:
                    return lex_error(LexErr::AddressNamingError);
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();

            }

        } else if (cur_ch == 'R') {

            switch (cur_state) {
                case State::Nil:
                    cur_state = State::Rgt;
                    break;
                case State::Rgt:
                case State::Reg:
                    cur_state = State::Idn;
                    break;
                case State::Adt:
                case State::Adr:
                    return lex_error(LexErr::AddressNamingError);
                    break;
                case State::Imt:
                case State::Imm:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbt:
                case State::Lbl:
                    cur_state = State::Lbl;
                    break;
                case State::Zer:
                    return lex_error(LexErr::IdentifierNamingError);
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
                default:
                    break;
            }

        } else if (cur_ch == '@') {
            
            switch (cur_state) {
                case State::Idn:
                case State::Rgt:
                case State::Reg:
                case State::Zer:
                    return lex_error(LexErr::IdentifierNamingError);
                    break;
                case State::Imt:
                case State::Imm:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbl:
                case State::Lbt:
                    return lex_error(LexErr::LabelNamingError);
                    break;
                case State::Adt:
                case State::Adr:
                    return lex_error(LexErr::AddressNamingError);
                    break;
                case State::Nil:
                    cur_state = State::Lbt;
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
            }

        } else if (cur_ch == '#') {

            switch (cur_state) {
                case State::Idn:
                case State::Rgt:
                case State::Reg:
                case State::Zer:
                    return lex_error(LexErr::IdentifierNamingError);
                    break;
                case State::Imt:
                case State::Imm:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbl:
                case State::Lbt:
                    return lex_error(LexErr::LabelNamingError);
                    break;
                case State::Adt:
                case State::Adr:
                    return lex_error(LexErr::AddressNamingError);
                    break;
                case State::Nil:
                    cur_state = State::Imt;
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
            }

        } else if (cur_ch == '_') {

            switch (cur_state) {
                case State::Idn:
                case State::Rgt:
                case State::Reg:
                case State::Nil:
                    cur_state = State::Idn;
                    break;
                case State::Zer:
                    return lex_error(LexErr::IdentifierNamingError);
                    break;
                case State::Imt:
                case State::Imm:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbl:
                case State::Lbt:
                    cur_state = State::Lbl;
                    break;
                case State::Adt:
                case State::Adr:
                    return lex_error(LexErr::AddressNamingError);
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
            }
            
        } else if (std::isdigit(cur_ch)) {

            switch (cur_state) {
                case State::Idn:
                    cur_state = State::Idn;
                    break;
                case State::Rgt:
                case State::Reg:
                    cur_state = State::Reg;
                    break;
                case State::Nil:
                    if (cur_ch == '0') { //if its zero, set to zer to transition to address, else raise error
                        cur_state = State::Zer;
                    } else {
                        return lex_error(LexErr::IdentifierNamingError);
                    }
                    break;
                case State::Imt:
                case State::Imm:
                    cur_state = State::Imm;
                    break;
                case State::Lbt:
                case State::Lbl:
                    cur_state = State::Lbl;
                    break;
                case State::Zer:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Adt:
                case State::Adr:
                    cur_state = State::Adr;
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
            }

        } else if (std::isalpha(cur_ch)) {

            switch (cur_state) {
                case State::Idn:
                case State::Rgt:
                case State::Reg:
                case State::Nil:
                    cur_state = State::Idn;
                    break;
                case State::Imm:
                case State::Imt:
                    return lex_error(LexErr::ImmediateValueError);
                    break;
                case State::Lbt:
                case State::Lbl:
                    cur_state = State::Lbl;
                    break;
                case State::Zer:
                    return lex_error(LexErr::IdentifierNamingError);
                    break;
                case State::Adt:
                case State::Adr:
                    if ((cur_ch >= 'A' && cur_ch <= 'F') || (cur_ch >= 'a' && cur_ch <= 'f')) { //if between A-F or a-f
                        cur_state = State::Adr;
                    } else {
                        return lex_error(LexErr::AddressNamingError);
                    }
                    break;
                case State::Err:
                case State::Sep: //the separator state is declared but never set
                    break;
                case State::Cmt:
                    panic::panic();
                
            }

        } else {

            cur_state = State::Err;
            return lex_error(LexErr::UnknownCharError);

        }

        return {};

    }

    std::expected<void, Error> Lexer::set_action() {

        switch ( state_map.at(prev_state) ) {

            case StateType::Word:
            case StateType::RTransition:
                switch ( state_map.at(cur_state) ) {
                    case StateType::Word:
                        cur_action = Action::Push;
                        break;
                    case StateType::None:
                        cur_action = Action::Emit;
                        break;
                    default: // default, anyways handled by error
                        cur_action = Action::Idle;
                        break;
                }
                break;

            case StateType::ZTransition:
                switch ( state_map.at(cur_state) ) { 
                    case StateType::Transition:
                        cur_action = Action::Push;
                        break;
                    default:
                        cur_action = Action::Idle;
                        break;
                }
                break;

            case StateType::Transition:
                switch ( state_map.at(cur_state) ) {
                    case StateType::Word:
                        cur_action = Action::Push;
                        break;
                    default:
                        cur_action = Action::Idle; // anyways handled by error
                        break;
                }
                break;
            case StateType::None:
                tkn_start_char_col = col_count;
                switch ( state_map.at(cur_state) ) {
                    case StateType::None:
                        cur_action = Action::Idle;
                        break;
                    default:
                        cur_action = Action::Push;
                        break;
                }
                break;
        }

        return {};

    }

    std::expected<void, Error> Lexer::execute(std::vector<instruction_mod::Token>& tokens) {

        instruction_mod::Token token;
        switch (cur_action) {

        case Action::Push:
            buffer.push_back(cur_ch);
            break;

        case Action::Emit:

            switch (prev_state) {
                case State::Idn:
                case State::Rgt:
                    {
                        auto opcode = instruction_mod::find_opcode(buffer);
                        if (opcode.has_value()) {
                            token = instruction_mod::Token(instruction_mod::OpCodeToken{*opcode}, line_count, tkn_start_char_col, col_count);
                        } else {
                            token = instruction_mod::Token(instruction_mod::VariableToken{buffer}, line_count, tkn_start_char_col, col_count);
                        }
                    }
                    break;
                case State::Reg:
                case State::Imm:
                    {
                        const char* start_char = buffer.data() + 1; //skip first character R or #
                        const char* end_char = buffer.data() + buffer.size();

                        int val = 0;
                        auto [ptr, errc] = std::from_chars(start_char, end_char, val);

                        if (errc == std::errc::invalid_argument || ptr != end_char) { //shouldnt happen ideally
                            token = instruction_mod::Token(instruction_mod::InvalidToken{}, line_count, tkn_start_char_col, col_count);
                        } else if (errc == std::errc::result_out_of_range) { //value is too big to parse
                            if (prev_state == State::Reg) return lex_error(LexErr::RegisterOutOfRange);
                            return lex_error(LexErr::ImmediateValueTooBig);
                        } else if (prev_state == State::Reg) { //success
                            token = instruction_mod::Token(instruction_mod::RegisterToken{val}, line_count, tkn_start_char_col, col_count);
                        } else {
                            token = instruction_mod::Token(instruction_mod::ImmediateToken{val}, line_count, tkn_start_char_col, col_count);
                        }
                    }
                    break;
                case State::Adr:
                    {
                        const char* start_char = buffer.data() + 2; //skip first two characters, 0x
                        const char* end_char = buffer.data() + buffer.size();

                        int val = 0;
                        auto [ptr, errc] = std::from_chars(start_char, end_char, val, 16); 

                        if (errc == std::errc::invalid_argument || ptr != end_char) { //shouldn't happen ideally
                            token = instruction_mod::Token(instruction_mod::InvalidToken{}, line_count, tkn_start_char_col, col_count);
                        } else if (errc == std::errc::result_out_of_range) { //too big to parse
                            return lex_error(LexErr::AddressOutOfRange);
                        } else { //success
                            token = instruction_mod::Token(instruction_mod::AddressToken{val}, line_count, tkn_start_char_col, col_count);
                        }
                    }
                    break;
                case State::Lbl:
                    {
                        token = instruction_mod::Token(instruction_mod::LabelToken{buffer.substr(1)}, line_count, tkn_start_char_col, col_count);
                    }
                    break;
                default:
                    {
                        panic::panic("execute: emitting a token from a state that has none");
                    }
            }

            tokens.push_back(std::move(token));
            buffer.clear();
            break;

        case Action::Idle:
            break;

        }

        return {};

    }

    LexResult Lexer::lex_line(std::string_view line, size_t line_no) {

        //every line starts from scratch, exactly like the old parser did after a newline
        cur_state = State::Nil;
        prev_state = State::Nil;
        cur_action = Action::Idle;
        buffer.clear();
        cur_ch = 0;
        line_count = line_no;
        col_count = 1;
        tkn_start_char_col = 0;

        LexResult result;

        auto step = [this, &result](char current_char) -> bool {
            cur_ch = current_char;
            if (cur_state != State::Cmt) { //characters inside a comment are ignored
                auto outcome = set_state()
                    .and_then([this]() { return set_action(); })
                    .and_then([this, &result]() { return execute(result.tokens); });
                if (!outcome) {
                    result.error = outcome.error(); //the rest of the line is ignored after an error
                    return false;
                }
            }
            ++col_count;
            prev_state = cur_state;
            return true;
        };

        for (char c : line) {
            if (!step(c)) return result;
        }
        step('\n'); //the end of the line acts like whitespace: it ends a token that is still being read

        return result;

    }

    //helper
    std::unexpected<Error> Lexer::lex_error(LexErr err) {
        return std::unexpected<Error>(Error(err, line_count, col_count));
    }

}
