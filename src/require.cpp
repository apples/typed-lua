#include "loader.hpp"

#include "typedlua_compiler.hpp"

namespace typedlua {

namespace { // static

const char* install_require_lua = R"(
    local tlua_get_type, name = ...

    local realname = name:gsub('%.', '/')

    for path in package.path:gmatch('[^;]+') do
        local filepath = path:gsub('?', realname)
        local file = io.open(filepath)

        if file then
            local text = file:read('*a')
            file:close()

            tlua_get_type(text)

            return
        end
    end
)";

int tlua_get_type(lua_State* L) {
    auto source = lua_tostring(L, 1);
    auto global_scope = static_cast<Scope*>(lua_touserdata(L, lua_upvalueindex(1)));
    auto result = static_cast<Type*>(lua_touserdata(L, lua_upvalueindex(2)));

    auto [root_node, errors] = typedlua::parse(source);

    if (root_node && errors.empty()) {
        auto scope = Scope(global_scope);
        scope.deduce_return_type();

        errors = typedlua::check(*root_node, scope);

        if (errors.empty()) {
            auto rettype = scope.get_return_type();

            if (rettype) {
                *result = *rettype;
            } else {
                *result = Type{};
            }
        }
    }

    return 0;
}

} // static

void install_require(lua_State* L, Scope& global_scope) {
    global_scope.set_get_package_type([L, &global_scope](const std::string& name){
        auto result = Type::make_any();

        luaL_loadstring(L, install_require_lua);
        lua_pushlightuserdata(L, &global_scope);
        lua_pushlightuserdata(L, &result);
        lua_pushcclosure(L, tlua_get_type, 2);
        lua_pushstring(L, name.c_str());

        auto err = lua_pcall(L, 2, 0, 0);

        if (err) {
            auto message = std::string(lua_tostring(L, -1));
            lua_pop(L, 1);
            throw std::runtime_error("Failed to get type of $require(" + name + "): " + message);
        }

        return result;
    });
}

} // namespace typedlua
