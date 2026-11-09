#pragma once

#include <vector>
#include <variant>
#include <concepts>
#include <string_view>
#include <ostream>

namespace error {

    enum class LexError { //found while reading the characters of a line
        ImmediateValueError,
        IdentifierNamingError,
        LabelNamingError,
        AddressNamingError,
        UnknownCharError,
        RegisterOutOfRange,
        ImmediateValueTooBig,
        AddressOutOfRange,
    };

    enum class ParseError { //found while building an instruction out of tokens
        InstructionTooLong,
    };

    enum class ClassifyError { //found while declaring labels and numbering instructions
        MissingOpCodeError,
        LabelAlreadyExists,
        LabelTooLongError,
    };

    enum class SemanticError { //found while checking that a program makes sense
        MissingOpCodeError,
        IncorrectFirstToken,
        IncorrectOperandFmt,
        UnknownSemanticError,
        InvalidOpCodePosition,
        RegisterOutOfRange,
        ImmediateOutOfRange,
        AddressOutOfRange,
        UndefinedLabelError,
    };

    enum class CodeGenError { //found while writing the binary
        OutputFileError,
    };

    template <typename T>
    concept Err = std::same_as<T, LexError> || std::same_as<T, ParseError> || std::same_as<T, ClassifyError>
        || std::same_as<T, SemanticError> || std::same_as<T, CodeGenError>;

    class Error {

        public:

            std::variant<LexError, ParseError, ClassifyError, SemanticError, CodeGenError> error;
            size_t err_line;
            size_t err_col;


            Error() = delete;

            Error(Err auto error, size_t err_ln, size_t err_cl) :
                error{error},
                err_line{err_ln},
                err_col{err_cl}
            { }

            std::string_view fmt_error_as_str();

    };

    class ErrorLog {

        private:

            std::vector<Error> errors;

        public:

            void log(Error err);
            bool has_errors() const;
            void display(std::ostream& out); //prints "Errors found:" and one line per error, in the order they were logged

    };

}