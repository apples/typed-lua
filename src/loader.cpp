#include "loader.hpp"

#include "typedlua_compiler.hpp"

namespace typedlua {

namespace { // static

const char* install_loader_lua = R"(
    local tlua_compile = ...

    local function loader(name)
        local realname = name:gsub('%.', '/')

        local errors = {}

        for path in package.path:gmatch('[^;]+') do
            local filepath = path:gsub('?', realname)
            local file = io.open(filepath)

            if file then
                local text = file:read('*a')
                file:close()

                local result, err = tlua_compile(text)

                if result then
                    return (loadstring or load)(result, name), filepath
                else
                    return '\n\t' .. filepath .. ': ' .. err
                end
            else
                errors[#errors + 1] = '\n\tno file \'' .. filepath .. '\''
            end
        end

        return table.unpack(errors)
    end

    local loaders = package.loaders or package.searchers

    loaders[2] = loader
)";

int tlua_compile(lua_State* L) {
    auto source = lua_tostring(L, 1);
    auto global_scope = static_cast<Scope*>(lua_touserdata(L, lua_upvalueindex(1)));

    auto new_source = std::optional<std::string>{};
    auto error_string = std::optional<std::string>{};

    auto root_node = typedlua::parse(source);

    if (root_node) {
        auto scope = Scope(global_scope);
        scope.deduce_return_type();

        auto errors = typedlua::check(*root_node, scope);

        if (!errors.empty()) {
            auto oss = std::ostringstream{};

            oss << "=== ERRORS ===\n";

            for (const auto& error : errors) {
                switch (error.severity) {
                    case typedlua::CompileError::Severity::ERROR:
                        oss << "ERROR: ";
                        break;
                    case typedlua::CompileError::Severity::WARNING:
                        oss << "Warning: ";
                        break;
                }

                oss << error.location.first_line << ": ";
                oss << error.message << "\n";
            }

            error_string = oss.str();
        } else {
            new_source = typedlua::compile(*root_node);
        }
    }

    if (new_source) {
        lua_pushstring(L, new_source->data());
    } else {
        lua_pushnil(L);
    }

    if (error_string) {
        lua_pushstring(L, error_string->data());
    } else {
        lua_pushnil(L);
    }

    return 2;
}

} // static

void install_loader(lua_State* L, Scope& global_scope) {
    luaL_loadstring(L, install_loader_lua);
    lua_pushlightuserdata(L, &global_scope);
    lua_pushcclosure(L, tlua_compile, 1);

    auto err = lua_pcall(L, 1, 0, 0);

    if (err) {
        auto message = std::string(lua_tostring(L, -1));
        lua_pop(L, 1);
        throw std::runtime_error("Failed to install typedlua loader: " + message);
    }
}

} // namespace typedlua
