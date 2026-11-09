#pragma once

#include <iostream>
#include <source_location>
#include <string_view>

namespace panic {
    [[noreturn]] void panic(std::string_view message = "unreachable", std::source_location loc = std::source_location::current());
}