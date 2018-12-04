#pragma once

#include <string>
#include <string_view>

class typedlua_compiler {
public:
    typedlua_compiler();

    auto compile(std::string_view source, std::string_view name) -> std::string;
private:
};
