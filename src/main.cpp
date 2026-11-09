#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>

#include "assembler/assembler.hpp"

namespace fs = std::filesystem;

// usage text for the command-line arguments
// ----------------------------------------------------------------
static void print_usage(std::ostream& out, const char* program) {
    out << "Usage: " << program << " [input.skasm] [-o output.bin]\n"
        << "  no arguments: reads progs/" << assembler_mod::prog_name << ".skasm and writes progs/" << assembler_mod::prog_name << ".bin\n"
        << "  -o FILE:      write the binary to FILE (default: the input name with the extension .bin)\n"
        << "  -h, --help:   show this text\n";
}
// ----------------------------------------------------------------

int main(int argc, char* argv[]) {

    fs::path filepath;
    fs::path output_path; // empty means the default output location
    bool interactive = (argc == 1);
    if (interactive) {
        fs::path source_dir = fs::path(__FILE__).parent_path();
        std::string filename = std::string(assembler_mod::prog_name) + ".skasm";
        filepath = source_dir.parent_path() / "progs" / filename;
    } else {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-h" || arg == "--help") {
                print_usage(std::cout, argv[0]);
                return 0;
            } else if (arg == "-o") {
                if (i + 1 >= argc) { std::cerr << "Error: -o needs a file name\n"; print_usage(std::cerr, argv[0]); return 2; }
                output_path = argv[++i];
            } else if (arg.size() > 1 && arg[0] == '-') {
                std::cerr << "Error: unknown option " << arg << "\n"; print_usage(std::cerr, argv[0]); return 2;
            } else if (filepath.empty()) {
                filepath = arg;
            } else {
                std::cerr << "Error: more than one input file given\n"; print_usage(std::cerr, argv[0]); return 2;
            }
        }
        if (filepath.empty()) { std::cerr << "Error: no input file given\n"; print_usage(std::cerr, argv[0]); return 2; }
        if (output_path.empty()) { output_path = filepath; output_path.replace_extension(".bin"); } // no -o: the input name with the extension .bin
        if (fs::weakly_canonical(output_path) == fs::weakly_canonical(filepath)) { std::cerr << "Error: the output file would overwrite the input file, use -o\n"; return 2; }
    }

    int exit_code = 0; // 0 = ok, 1 = the program has errors, 2 = input file not found
    std::ifstream asm_prog(filepath);
    
    if (asm_prog) {
        std::cout << "File found: " << filepath.filename() << std::endl;

        std::stringstream buffer;
        buffer << asm_prog.rdbuf();

        assembler_mod::Assembler assembler{};
        assembler.set_output_path(output_path); // pass on the -o option (empty means the default location)
        std::vector<instruction_mod::Inst> inst_pipeline{};
        
        assembler.assemble_prog(inst_pipeline, buffer.str());
        if (assembler.has_errors()) exit_code = 1; // report errors through the exit code
        if (assembler.has_output_error()) exit_code = 2;
        assembler.dbg_display_tokens(inst_pipeline);
        assembler.dbg_display_labels();
        
    } else {
        std::cout << "Error: File not found!" << std::endl;
        std::cout << "Path searched: " << fs::absolute(filepath) << std::endl;
        exit_code = 2; // input file not found
    }

    if (interactive) { // only wait for a key press when started without arguments, so scripts do not hang
        std::cout << "Press any key to exit..." << std::endl;
        std::cin.get();
    }

    return exit_code;
}