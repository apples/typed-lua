
#include "sol.hpp"
#include <loader.hpp>

int main() {
    sol::state lua;
    lua.open_libraries(
        sol::lib::base,
        sol::lib::io,
        sol::lib::string,
        sol::lib::table,
        sol::lib::package);
    
    lua["package"]["tluapath"] = "?.lua";

    auto deferred = typedlua::DeferredTypeCollection();
    auto scope = typedlua::Scope(&deferred);

    scope.enable_basic_types();

    typedlua::install_loader(lua.lua_state(), scope);

    lua.script(R"(
        local simple = require('simple')
        simple.howdy()
    )");
}
