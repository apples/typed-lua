#pragma once

#include "location.hpp"

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
    Location location;

    CompileError() = default;
    CompileError(std::string m, Location l) : message(std::move(m)), location(l) {}
    CompileError(Severity s, std::string m, Location l) : severity(s), message(std::move(m)), location(l) {}
};

} // namespace typedlua
