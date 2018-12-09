#pragma once

#include "type.hpp"

#include <unordered_map>
#include <string>
#include <optional>

namespace typedlua {

class Scope {
public:
    Scope() = default;
    Scope(Scope* parent) : parent(parent) {}

    const Type* get_type_of(const std::string& name) const {
        auto iter = names.find(name);
        if (iter != names.end()) {
            return &iter->second;
        } else if (parent) {
            return parent->get_type_of(name);
        } else {
            return nullptr;
        }
    }

    void add_name(const std::string& name, Type type) {
        names[name] = std::move(type);
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

private:
    enum class DotsState {
        INHERIT,
        NONE,
        OWN
    };

    Scope* parent = nullptr;
    std::unordered_map<std::string, Type> names;
    std::optional<Type> dots_type;
    DotsState dots_state = DotsState::INHERIT;
};

} // namespace typedlua
