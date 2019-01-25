#pragma once

#include "scope.hpp"

namespace typedlua::libs {

void import_basic(Scope& scope);

void import_math(Scope& scope);

void import_string(Scope& scope);

} // namespace typedlua::libs
