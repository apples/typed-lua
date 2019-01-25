#pragma once

#include "location.hpp"

#include <string>
#include <utility>
#include <iostream>
#include <vector>

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

inline std::ostream& operator<<(std::ostream& out, const CompileError& error) {
    switch (error.severity) {
        case typedlua::CompileError::Severity::ERROR:
            out << "Error: ";
            break;
        case typedlua::CompileError::Severity::WARNING:
            out << "Warning: ";
            break;
    }

    out << error.location.first_line << "," << error.location.first_column << ": ";
    out << error.message << "\n";

    return out;
}

inline std::ostream& operator<<(std::ostream& out, const std::vector<CompileError>& errors) {
    for (const auto& error : errors) {
        out << error;
    }
    return out;
}

} // namespace typedlua
