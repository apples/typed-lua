#pragma once

#include "compile_error.hpp"
#include "scope.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace typedlua {

class Compiler {
public:
    struct Result {
        std::string new_source;
        std::vector<CompileError> errors;
    };

    Compiler();
    Compiler(Scope& global_scope);

    auto compile(std::string_view source, std::string_view name) -> Result;

private:
    Scope* global_scope;
};

} // namespace typedlua
