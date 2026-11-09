#include "assembler.hpp"
#include "../overload.hpp"

using namespace assembler_mod;

Assembler::Assembler() :
    error_log{},
    output_path{},
    symbols{}
{ }

void Assembler::assemble_prog(instruction_mod::Pipeline& pipeline, const std::string& file) {

    parse_source(pipeline, file);

    if (!pipeline.empty()) {
        classify(pipeline); //every instruction is classified before any is analyzed, so labels can be used before they are defined
        analyze(pipeline);
    }

    if (!has_errors()) { //only do codegen if no errors are detected
        generate(pipeline);
    } else {
        std::cout << "Errors detected. Code generation did not start.\n";
        display_errors();
    }

}

void Assembler::parse_source(instruction_mod::Pipeline& pipeline, const std::string& file) {
    auto parser = parser_mod::Parser();
    parser.parse_source(file, pipeline, error_log);
}

void Assembler::classify(instruction_mod::Pipeline& pipeline) {
    auto classifier = classifier_mod::Classifier();
    for (auto& inst : pipeline) {
        auto classify_success = classifier.classify(inst, symbols);
        if (!classify_success) error_log.log(classify_success.error());
    }
}

void Assembler::analyze(const instruction_mod::Pipeline& pipeline) {
    auto analyzer = analyzer_mod::Analyzer();
    for (const auto& inst : pipeline) {
        auto analyze_success = analyzer.analyze(inst, symbols);
        if (!analyze_success) error_log.log(analyze_success.error());
    }
}

void Assembler::generate(const instruction_mod::Pipeline& pipeline) {
    std::cout << "Errors not detected. Code generation started.\n";
    layout_mod::Layout().run(pipeline, symbols);
    auto codegen = codegen_mod::CodeGen(symbols);
    fs::path source_dir = fs::path(__FILE__).parent_path().parent_path().parent_path();
    std::string filename = std::string(assembler_mod::prog_name) + ".bin";
    fs::path filepath = source_dir / "progs" / filename;
    if (!output_path.empty()) filepath = output_path; // -o replaces the default output location
    std::ofstream output(filepath, std::ios::binary);
    if (!output) { // report an output file that cannot be opened, instead of silently writing nothing
        std::cout << "Error: " << error::Error(error::CodeGenError::OutputFileError, 0, 0).fmt_error_as_str() << " " << filepath << "\n";
        output_error = true;
        return;
    }
    codegen.write_magic(output, MAGIC_NUM);
    for (const auto& inst : pipeline) {
        codegen.generate(output, inst);
    }
    std::cout << "Code generation successfully completed.\n";
}

void Assembler::dbg_display_tokens(const instruction_mod::Pipeline& pipeline) const {

    for (size_t i{0}; i < pipeline.size(); ++i) {
        const auto& cur_inst{pipeline[i]};
        std::cout << "Instruction: " << i << "(" << magic_enum::enum_name(cur_inst.inst_type) << ")" << ": ";
        for (size_t j{0}; j < instruction_mod::INST_SIZE; ++j) {
            const auto& cur_token{cur_inst.token_arr[j]};
            if (cur_token.has_value()) {
                std::cout << instruction_mod::token_type_name(*cur_token) << " : ";
                std::visit(overload {
                    [](const instruction_mod::OpCodeToken& t) { std::cout << magic_enum::enum_name(t.value); }, //for opcode
                    [](const instruction_mod::LabelToken& t) { std::cout << t.name; },
                    [](const instruction_mod::VariableToken& t) { std::cout << t.name; },
                    [](const instruction_mod::InvalidToken&) { std::cout << 0; },
                    [](const auto& t) { std::cout << t.value; }, //for register, immediate and address
                }, cur_token->kind);
            } else {
                std::cout << "No Token";
            }
            std::cout << " , ";
        }
        std::cout << '\n';
    }

}

void Assembler::dbg_display_labels() const {

    for (const auto& name : symbols.label_names()) {
        const size_t address = symbols.label_address(name).value_or(symbols.label_inst_index(name) * instruction_mod::INST_BYTES);
        std::cout << name << ": " << address << "\n";
    }

}

void Assembler::display_errors() {
    error_log.display(std::cout);
}