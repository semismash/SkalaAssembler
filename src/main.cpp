#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>

#include "assembler/assembler.hpp"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    fs::path source_dir = fs::path(__FILE__).parent_path();
    std::string filename = std::string(assembler_mod::prog_name) + ".skasm";
    fs::path filepath = source_dir.parent_path() / "progs" / filename;

    int exit_code = 0; // 0 = ok, 1 = the program has errors, 2 = input file not found
    std::ifstream asm_prog(filepath);
    
    if (asm_prog) {
        std::cout << "File found: " << filepath.filename() << std::endl;

        std::stringstream buffer;
        buffer << asm_prog.rdbuf();

        assembler_mod::Assembler assembler{};
        std::vector<instruction_mod::Inst> inst_pipeline{};
        
        assembler.assemble_prog(inst_pipeline, buffer.str());
        if (assembler.has_errors()) exit_code = 1; // report errors through the exit code
        assembler.dbg_display_tokens(inst_pipeline);
        assembler.dbg_display_labels();
        
    } else {
        std::cout << "Error: File not found!" << std::endl;
        std::cout << "Path searched: " << fs::absolute(filepath) << std::endl;
        exit_code = 2; // input file not found
    }

    std::cout << "Press any key to exit..." << std::endl;
    std::cin.get();

    return exit_code;   // return exit code now
}