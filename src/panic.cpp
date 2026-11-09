#include "panic.hpp"
#include <cstdlib> 

void panic::panic(std::string_view message, std::source_location loc) {
    std::cerr << 
    "An unexpected error has occured in the program, due to a bug causing unintended control flow. Please check your code." 
    << std::endl;
    std::cerr << "  " << message << " (" << loc.file_name() << ":" << loc.line() << ")" << std::endl;
    std::abort();
}