#pragma once

#include <string>
#include <utility>

namespace typedlua {

struct CompileError {
    enum class Severity {
        ERROR,
        WARNING
    };

    Severity severity = Severity::ERROR;
    std::string message;

    CompileError() = default;
    CompileError(std::string m) : message(std::move(m)) {}
    CompileError(Severity s, std::string m) : severity(s), message(std::move(m)) {}
};

} // namespace typedlua
