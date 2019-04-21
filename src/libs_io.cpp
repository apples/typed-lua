#include "libs.hpp"

#include "type.hpp"
#include "typedlua_compiler.hpp"

#include <sstream>
#include <stdexcept>

namespace typedlua::libs {

void import_io(Scope& scope) {
    auto [node, errors] = parse(R"(
        interface file: {
            close: (): void
            flush: (): void
            lines: (...): [:(:any, :any): string, :any, :any]
            read: (...): [...]
            seek:
                ((): number) &
                ((whence: 'set' | 'cur' | 'end'): number) &
                ((whence: 'set' | 'cur' | 'end', offset: number): number)
            setvbuf:
                ((mode: 'no'): void) &
                ((mode: 'full' | 'line'): void) &
                ((mode: 'full' | 'line', size: number): void)
            write: (...): file
        }

        interface open_mode: 'r' | 'rb' | 'w' | 'wb' | 'a' | 'ab' | 'r+' | 'r+b' | 'w+' | 'w+b' | 'a+' | 'a+b';

        global io: {
            close: (file: file | nil): void
            flush: (): void
            input:
                ((): file) &
                ((file: string | file): void)
            lines:
                ((): [:(:any, :any): string, :any, :any]) &
                ((filename: string): [:(:any, :any): string, :any, :any])
            open: (filename: string, mode: open_mode | nil): file
            output:
                ((): file) &
                ((file: string | file): void)
            popen: (prog: string, mode: 'r' | 'w' | nil): file
            read: (...): [...]
            tmpfile: (): file
            type: (obj: file): 'file' | 'closed file'
            write: (...): file | [:nil, error: string]
        }
    )");

    if (node && errors.empty()) {
        errors = check(*node, scope);
    }

    if (!errors.empty()) {
        std::ostringstream oss;

        oss << errors;

        throw std::logic_error("Error: import_io: " + oss.str());
    }
}

} // namespace typedlua::libs
