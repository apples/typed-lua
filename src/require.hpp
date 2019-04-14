#pragma once

#include "scope.hpp"

#include "lua.hpp"

namespace typedlua {

void install_require(lua_State* L, Scope& scope);

} // namespace typedlua
