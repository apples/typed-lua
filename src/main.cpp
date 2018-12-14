#include <iostream>
#include <sstream>

#include "typedlua_compiler.hpp"

int main(int argc, char **argv) {
    auto deferred_types = typedlua::DeferredTypeCollection{};
    auto scope = typedlua::Scope(&deferred_types);
    scope.enable_basic_types();
    auto tlc = typedlua::Compiler(scope);
    
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
        std::cout << error.location.last_line << ": ";
        std::cout << error.message << std::endl;
    }
}
