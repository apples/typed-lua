#include <iostream>
#include <sstream>

#include "typedlua_compiler.hpp"
#include "libs.hpp"

int main(int argc, char **argv) {
    auto ss = std::stringstream{};

    ss << std::cin.rdbuf();

    auto [root_node, errors] = typedlua::parse(ss.str());

    if (root_node && errors.empty()) {
        auto deferred_types = typedlua::DeferredTypeCollection{};
        auto scope = typedlua::Scope(&deferred_types);

        scope.enable_basic_types();
        typedlua::libs::import_basic(scope);
        typedlua::libs::import_math(scope);

        errors = typedlua::check(*root_node, scope);
        auto new_source = typedlua::compile(*root_node);

        std::cout << new_source;
    }

    if (!errors.empty()) {
        std::cout << "=== ERRORS ===\n" << errors;
    }
}
