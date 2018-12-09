#pragma once

#include "compile_error.hpp"

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

    auto compile(std::string_view source, std::string_view name) -> Result;
private:
};

} // namespace typedlua
