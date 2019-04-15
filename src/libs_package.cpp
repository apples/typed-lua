#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_package(Scope& scope) {
    auto [node, errors] = parse(R"(
        global require: <T: string>(modname: T): $require(T)

        global package: {
            config: string
            cpath: string
            loaded: { [string]: any }
            loadlib: (libname: string, funcname: string): any
            path: string
            preload: {
                [string]: (modname: string): [loader: (arg: any): any, arg: any] | string | nil
            }
            searchers: {
                [string]: (modname: string): [loader: (arg: any): any, arg: any] | string | nil
            }
            searchpath: (name: string, path: string, sep: string | nil, rep: string | nil): string | [:nil, error: string]
        }
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
