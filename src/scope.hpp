#pragma once

#include "type.hpp"

#include <unordered_map>
#include <string>
#include <optional>

namespace typedlua {

class Scope {
public:
    Scope() = default;
    Scope(DeferredTypeCollection* dt) : deferred_types(dt) {}
    Scope(Scope* parent) : parent(parent) {}

    const Type* get_type_of(const std::string& name) const {
        auto iter = names.find(name);
        
        if (iter != names.end())
        {
            return &iter->second;
        }
        else if (parent)
        {
            return parent->get_type_of(name);
        }
        else
        {
            return nullptr;
        }
    }

    void add_name(const std::string& name, Type type) {
        names.insert_or_assign(name, std::move(type));
    }

    void add_global_name(const std::string& name, Type type) {
        if (parent) {
            parent->add_global_name(name, std::move(type));
        } else {
            names.insert_or_assign(name, std::move(type));
        }
    }

    const Type* get_dots_type() const {
        switch (dots_state) {
            case DotsState::INHERIT: return parent->get_dots_type();
            case DotsState::NONE: return nullptr;
            case DotsState::OWN: return &*dots_type;
            default: throw std::logic_error("DotsState not supported");
        }
    }

    void set_dots_type(Type type) {
        dots_type = std::move(type);
        dots_state = DotsState::OWN;
    }

    void disable_dots() {
        dots_type = std::nullopt;
        dots_state = DotsState::NONE;
    }

    const Type* get_type(const std::string& name) const {
        auto iter = types.find(name);
        if (iter != types.end()) {
            return &iter->second;
        } else if (parent) {
            return parent->get_type(name);
        } else {
            return nullptr;
        }
    }

    void add_type(const std::string& name, Type type) {
        types[name] = std::move(type);
    }

    void enable_basic_types() {
        add_type("void", Type());
        add_type("any", Type::make_any());
        add_type("nil", Type::make_luatype(LuaType::NIL));
        add_type("number", Type::make_luatype(LuaType::NUMBER));
        add_type("string", Type::make_luatype(LuaType::STRING));
        add_type("boolean", Type::make_luatype(LuaType::BOOLEAN));
        add_type("thread", Type::make_luatype(LuaType::THREAD));
    }

    const Type* get_fixed_return_type() const {
        switch (return_type_state) {
            case ReturnState::INHERIT:
                return parent->get_fixed_return_type();
            case ReturnState::FIXED:
                if (return_type) {
                    return &*return_type;
                } else {
                    return nullptr;
                }
            case ReturnState::DEDUCE:
                return nullptr;
            default:
                return nullptr;
        }
    }

    Type* get_return_type() {
        switch (return_type_state) {
            case ReturnState::INHERIT:
                return parent->get_return_type();
            case ReturnState::DEDUCE:
                return return_type ? &*return_type : nullptr;
            case ReturnState::FIXED:
                return &*return_type;
            default:
                return nullptr;
        }
    }

    void add_return_type(const Type& type) {
        switch (return_type_state) {
            case ReturnState::INHERIT:
                parent->add_return_type(type);
                break;
            case ReturnState::DEDUCE:
                if (return_type) {
                    return_type = *return_type | type;
                } else {
                    return_type = type;
                }
                break;
            case ReturnState::FIXED:
                throw std::logic_error("Cannot change fixed return type");
        }
    }

    void set_return_type(const Type& type) {
        return_type = type;
        return_type_state = ReturnState::FIXED;
    }

    void deduce_return_type() {
        return_type_state = ReturnState::DEDUCE;
    }

    DeferredTypeCollection& get_deferred_types() {
        if (deferred_types) {
            return *deferred_types;
        } else if (parent) {
            return parent->get_deferred_types();
        } else {
            throw std::logic_error("No deferred type collection in tree");
        }
    }

    void set_luatype_metatable(LuaType luatype, Type type) {
        if (parent) {
            throw std::logic_error("LuaType metatables can only be set on root scope");
        }

        luatype_metatables.insert_or_assign(luatype, std::move(type));
    }

    const Type* get_luatype_metatable(LuaType luatype) const {
        if (parent) {
            return parent->get_luatype_metatable(luatype);
        }

        auto iter = luatype_metatables.find(luatype);

        if (iter != luatype_metatables.end()) {
            return &iter->second;
        }

        return nullptr;
    }

    const std::unordered_map<LuaType, Type>& get_luatype_metatable_map() const {
        if (parent) {
            return parent->get_luatype_metatable_map();
        }

        return luatype_metatables;
    }

private:
    enum class DotsState {
        INHERIT,
        NONE,
        OWN
    };

    enum class ReturnState {
        INHERIT,
        FIXED,
        DEDUCE
    };

    Scope* parent = nullptr;
    std::unordered_map<std::string, Type> names;
    std::optional<Type> dots_type;
    DotsState dots_state = DotsState::INHERIT;
    std::unordered_map<std::string, Type> types;
    std::optional<Type> return_type;
    ReturnState return_type_state = ReturnState::INHERIT;
    DeferredTypeCollection* deferred_types = nullptr;
    std::unordered_map<LuaType, Type> luatype_metatables;
};

} // namespace typedlua
