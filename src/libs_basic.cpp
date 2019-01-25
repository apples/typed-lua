#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_basic(Scope& scope) {
    auto [node, errors] = parse(R"(
        global assert: <T,U>(v: T, message: U): [v: T, message: U]

        global collectgarbage: (
            opt: nil
                |'collect'
                |'stop'
                |'restart'
                |'count'
                |'step'
                |'setpause'
                |'setstepmul'
                |'isrunning',
            arg: nil|number): nil|number|boolean
        
        global dofile: (filename: nil|string): [...]

        global error: <T>(message: T, level: nil|number): void

        global _G: { [string]: any }

        global getmetatable: (object: any): any

        global ipairs: <V, T: {[number]: V}>(t: T): [
            f: (:T, :number):[:number, :V],
            s: T,
            var: number]
        
        global load: (
            chunk: string|(():string|nil),
            chunkname: nil|string,
            mode: nil|'b'|'t'|'bt',
            env: any): (...): [...]
       
        global loadfile: (
            filename: nil|string,
            mode: nil|'b'|'t'|'bt',
            env: any): (...): [...]
        
        global next: (table: any, index: nil|number): [index: number, value: any]

        global pairs: <T>(t: T): [
            f: (:T, :number):[:number, :any],
            s: T,
            var: any]
        
        global pcall: (f: any, ...): [:boolean, ...]

        global print: (...): void

        global rawequal: (v1: any, v2: any): boolean

        global rawget: (table: any, index: number): any

        global rawlen: (v: any): number

        global rawset: <T>(table: T, index: any, value: any): T

        global select: (index: '#'|number, ...): number|[...]

        global setmetatable: <T>(table: T, metatable: any): T

        global tonumber: (e: any, base: nil|number): nil|number

        global tostring: (v: any): string

        global type: (v: any):
            'nil'
            |'number'
            |'string'
            |'boolean'
            |'table'
            |'function'
            |'thread'
            |'userdata'
        
        global _VERSION: string

        global xpcall: (f: any, msgh: any, ...): [...]
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_basic: " + oss.str());
    }
}

} // namespace typedlua::libs
