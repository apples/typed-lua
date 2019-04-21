#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_table(Scope& scope) {
    auto [node, errors] = parse(R"(
        interface list: { [number]: string | number }

        global table: {
            concat: (list: list, sep: string | nil, i: number | nil, j: number | nil): string
            insert: ((list: list, value: any): void) & ((list: list, pos: number, value: any): void)
        }
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_table: " + oss.str());
    }
}

} // namespace typedlua::libs
