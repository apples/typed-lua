#include <iostream>
#include <sstream>

#include "typedlua_compiler.hpp"

int main(int argc, char **argv) {
    auto tlc = typedlua::Compiler();
    
    std::stringstream ss;
    ss << std::cin.rdbuf();

    auto result = tlc.compile(ss.str(), "cin");
    std::cout << result.new_source;

    std::cout << "=== ERRORS ===\n";

    for (const auto& error : result.errors) {
        switch (error.severity) {
            case typedlua::CompileError::Severity::ERROR:
                std::cout << "ERROR: ";
                break;
            case typedlua::CompileError::Severity::WARNING:
                std::cout << "Warning: ";
                break;
        }
        std::cout << error.message << std::endl;
    }
}
