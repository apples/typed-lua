#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_table(Scope& scope) {
    auto [node, errors] = parse(R"(
        interface list<T>: { [number]: T }

        global table: {
            concat:
                ((list: list<string | number>): string) &
                ((list: list<string | number>, sep: string): string) &
                ((list: list<string | number>, sep: string, i: number): string) &
                ((list: list<string | number>, sep: string, i: number, j: number): string)
            insert:
                (<T>(list: list<T>, value: T): void) &
                (<T>(list: list<T>, pos: number, value: T): void)
            move:
                (<T: {}>(a1: T, f: number, e: number, t: number): T) &
                (<T: {}, U: {}>(a1: T, f: number, e: number, t: number, a2: U): U)
            pack: (...): { [number]: any; n: number }
            remove:
                (<T>(list: list<T>): T) &
                (<T>(list: list<T>, pos: number): T)
            sort:
                ((list: list<any>): void) &
                (<T>(list: list<T>, comp: (l: T, r: T): boolean): void)
            unpack:
                ((list: list<any>): [...]) &
                ((list: list<any>, i: number): [...]) &
                ((list: list<any>, i: number, j: number): [...])
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
