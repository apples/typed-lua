#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_package(Scope& scope) {
    auto [node, errors] = parse(R"(
        global require: <T: string>(modname: T): $require(T)
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_package: " + oss.str());
    }
}

} // namespace typedlua::libs
