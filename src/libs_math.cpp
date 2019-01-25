#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_math(Scope& scope) {
    auto [node, errors] = parse(R"(
        global math: {
            abs: (x: number): number
            acos: (x: number): number
            asin: (x: number): number
            atan: (y: number, x: nil|number): number
            ceil: (x: number): number
            cos: (x: number): number
            deg: (x: number): number
            exp: (x: number): number
            floor: (x: number): number
            fmod: (x: number, y: number): number
            huge: number
            log: (x: number, base: number): number
            max: (x: number, ...): number
            maxinteger: number
            min: (x: number, ...): number
            mininteger: number
            modf: (x: number): [integral: number, fractional: number]
            pi: number
            rad: (x: number): number
            random: (m: nil|number, n: nil|number): number
            randomseed: (x: number): void
            sin: (x: number): number
            sqrt: (x: number): number
            tan: (x: number): number
            tointeger: (x: number): nil|number
            type: (x: number): nil|'integer'|'float'
            ult: (m: number, n: number): boolean
        }
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_math: " + oss.str());
    }
}

} // namespace typedlua::libs
