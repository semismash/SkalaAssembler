#include "error.hpp"
#include "../overload.hpp"

using namespace error;

std::string_view Error::fmt_error_as_str() {

    return std::visit(overload{
        [](LexError LErr){
            switch (LErr) {
                case LexError::ImmediateValueError:
                    return "Operand is not a correct immediate value";
                case LexError::IdentifierNamingError:
                    return "Identifier is not named correctly";
                case LexError::LabelNamingError:
                    return "Incorrect syntax for naming label";
                case LexError::AddressNamingError:
                    return "Incorrect syntax for naming address label";
                case LexError::UnknownCharError:
                    return "Unknown character, cannot be parsed";
                case LexError::RegisterOutOfRange:
                    return "Requested register number is too large";
                case LexError::ImmediateValueTooBig:
                    return "Immediate value is too large to be parsed";
                case LexError::AddressOutOfRange:
                    return "Address value is too large to be parsed";
                default:
                    return "Error message for this type has not been implemented";
            }
        },
        [](ParseError PErr){
            switch (PErr) {
                case ParseError::InstructionTooLong:
                    return "Input instruction has too many operands";
                default:
                    return "Error message for this type has not been implemented";
            }
        },
        [](ClassifyError CErr){
            switch (CErr) {
                case ClassifyError::MissingOpCodeError:
                    return "Missing OpCode in Instruction";
                case ClassifyError::LabelAlreadyExists:
                    return "Specified Label has already been defined";
                case ClassifyError::LabelTooLongError:
                    return "Specified Label should not have extra operands";
                default:
                    return "Error message for this type has not been implemented";
            }
        },
        [](SemanticError SErr){
            switch(SErr) {
                case SemanticError::MissingOpCodeError:
                    return "Missing OpCode in Instruction";
                case SemanticError::IncorrectFirstToken:
                    return "Instruction must either start with an OpCode or a Label";
                case SemanticError::IncorrectOperandFmt:
                    return "Operands not in the correct format as the given instructions";
                case SemanticError::InvalidOpCodePosition:
                    return "Opcode position is invalid";
                case SemanticError::RegisterOutOfRange:
                    return "Requested register number is out of range";
                case SemanticError::ImmediateOutOfRange:
                    return "Immediate value in instruction is out of range";
                case SemanticError::AddressOutOfRange:
                    return "Memory address in instruction is out of range";
                case SemanticError::UndefinedLabelError:
                    return "Specified Label has not yet been defined, but is used";
                case SemanticError::UnknownSemanticError:
                    return "Unknown semantic error";
                default:
                    return "Error message for this type has not been implemented";
            }
        },
        [](CodeGenError GErr){
            switch (GErr) {
                case CodeGenError::OutputFileError:
                    return "could not open output file";
                default:
                    return "Error message for this type has not been implemented";
            }
        }
    }, this->error);

}

void ErrorLog::log(Error err) {
    errors.push_back(std::move(err));
}

bool ErrorLog::has_errors() const {
    return !errors.empty();
}

void ErrorLog::display(std::ostream& out) {
    out << "Errors found: \n";
    for (auto& err : errors) {
        out <<
        "Line: " << err.err_line <<
        ", Column: " << err.err_col <<
        ": " << err.fmt_error_as_str() << "\n";
    }
}