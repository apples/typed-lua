#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_string(Scope& scope) {
    auto [node, errors] = parse(R"(
        global string: {
            byte: (s: string, i: nil|number, j: nil|number): [...]
            char: (...): string
            dump: (funct: any, strip: boolean): string
            find: (s: string, pattern: string, init: nil|number, plain: boolean): [s: number, e: number, ...]
            format: (formatstring: string, ...): string
            gmatch: (s: string, pattern: string): [
                f: (:string, :any):[:any, ...],
                s: string,
                var: any]
            gsub: (s: string, pattern: string, repl: any, n: nil|number): [s: string, n: number]
            len: (s: string): number
            lower: (s: string): string
            match: (s: string, pattern: string, init: nil|number): [...]
            pack: (fmt: string, ...): string
            packsize: (fmt: string): number
            rep: (s: string, n: number, sep: string): string
            reverse: (s: string): string
            sub: (s: string, i: number, j: nil|number): string
            unpack: (fmt: string, s: string, pos: nil|number): [...]
            upper: (s: string): string
        }
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_string: " + oss.str());
    }

    auto string_type = scope.get_type_of("string");

    if (!string_type) {
        throw std::logic_error("Error: import_string: string table missing");
    }

    scope.set_luatype_metatable(LuaType::STRING, *string_type);
}

} // namespace typedlua::libs
