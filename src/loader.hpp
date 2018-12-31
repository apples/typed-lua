#pragma once

#include "scope.hpp"

#include "lua.hpp"

namespace typedlua {

void install_loader(lua_State* L, Scope& scope);

} // namespace typedlua
