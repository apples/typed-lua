#include <iostream>
#include <sstream>

#include "typedlua_compiler.hpp"

int main(int argc, char **argv) {
    auto ss = std::stringstream{};

    ss << std::cin.rdbuf();

    auto root_node = typedlua::parse(ss.str());

    if (root_node) {
        auto deferred_types = typedlua::DeferredTypeCollection{};
        auto scope = typedlua::Scope(&deferred_types);

        scope.enable_basic_types();

        auto errors = typedlua::check(*root_node, scope);
        auto new_source = typedlua::compile(*root_node);

        std::cout << new_source;

        if (!errors.empty()) {
            std::cout << "=== ERRORS ===\n";

            for (const auto& error : errors) {
                switch (error.severity) {
                    case typedlua::CompileError::Severity::ERROR:
                        std::cout << "ERROR: ";
                        break;
                    case typedlua::CompileError::Severity::WARNING:
                        std::cout << "Warning: ";
                        break;
                }

                std::cout << error.location.first_line << ": ";
                std::cout << error.message << std::endl;
            }
        }
    }
}
