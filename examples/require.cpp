
#include "sol.hpp"
#include <loader.hpp>
#include <require.hpp>
#include <libs.hpp>

int main() {
    sol::state lua;
    lua.open_libraries(
        sol::lib::base,
        sol::lib::io,
        sol::lib::string,
        sol::lib::table,
        sol::lib::package);
    
    lua["package"]["path"] = "?.lua";

    auto deferred = typedlua::DeferredTypeCollection();
    auto scope = typedlua::Scope(&deferred);

    scope.enable_basic_types();

    typedlua::install_loader(lua.lua_state(), scope);
    typedlua::install_require(lua.lua_state(), scope);

    typedlua::libs::import_package(scope);

    lua.script(R"(
        local testsimple = require('testsimple')
        testsimple.test()
    )");
}
