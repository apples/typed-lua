#pragma once

#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>
#include <sstream>

namespace typedlua {

enum class LuaType {
    NIL,
    NUMBER,
    STRING,
    BOOLEAN,
    THREAD
};

class Type;

struct FunctionType {
    std::vector<Type> params;
    std::unique_ptr<Type> ret;

    FunctionType() = default;
    FunctionType(std::vector<Type> params, std::unique_ptr<Type> ret) :
        params(std::move(params)),
        ret(std::move(ret)) {}
    FunctionType(FunctionType&&) = default;
    FunctionType(const FunctionType& other) :
        params(other.params),
        ret(std::make_unique<Type>(*other.ret)) {}
};

class Type {
public:
    enum class Tag {
        VOID,
        ANY,
        LUATYPE,
        FUNCTION
    };

    Type() : tag(Tag::VOID) {}
    Type(LuaType lt) : tag(Tag::LUATYPE), luatype(lt) {}

    Type(const Type& other) : tag(other.tag) {
        assign_from(other);
    }

    Type(Type&& other) : tag(std::exchange(other.tag, Tag::VOID)) {
        assign_from(std::move(other));
    }

    Type& operator=(const Type& other) {
        destroy();
        tag = other.tag;
        assign_from(other);
        return *this;
    }

    Type& operator=(Type&& other) {
        destroy();
        tag = std::exchange(other.tag, Tag::VOID);
        assign_from(std::move(other));
        return *this;
    }

    ~Type() noexcept(false) {
        destroy();
    }

    static Type make_any() {
        auto type = Type{};
        type.tag = Tag::ANY;
        return type;
    }

    static Type make_luatype(LuaType lt) {
        auto type = Type();
        type.tag = Tag::LUATYPE;
        type.luatype = lt;
        return type;
    }

    static Type make_function(std::vector<Type> params, Type ret) {
        auto type = Type{};
        type.tag = Tag::FUNCTION;
        new (&type.function) FunctionType{std::move(params), std::make_unique<Type>(std::move(ret))};
        return type;
    }

    const Tag& get_tag() const { return tag; }

    const LuaType& get_luatype() const { return luatype; }

    const FunctionType& get_function() const { return function; }

private:
    void destroy() {
        switch (tag) {
            case Tag::VOID: break;
            case Tag::ANY: break;
            case Tag::LUATYPE: break;
            case Tag::FUNCTION: std::destroy_at(&function); break;
            default: throw std::logic_error("Type tag not implemented");
        }
    }

    void assign_from(const Type& other) {
        switch (tag) {
            case Tag::VOID: break;
            case Tag::ANY: break;
            case Tag::LUATYPE: luatype = other.luatype; break;
            case Tag::FUNCTION: new (&function) FunctionType(other.function); break;
            default: throw std::logic_error("Type tag not implemented");
        }
    }

    void assign_from(Type&& other) {
        switch (tag) {
            case Tag::FUNCTION: new (&function) FunctionType(std::move(other.function)); break;
            default: assign_from(other);
        }
    }

    Tag tag;
    union {
        LuaType luatype;
        FunctionType function;
    };
};

inline std::string to_string(const Type& type);

inline std::string to_string(const LuaType& luatype) {
    switch (luatype) {
        case LuaType::NIL: return "nil";
        case LuaType::NUMBER: return "number";
        case LuaType::STRING: return "string";
        case LuaType::BOOLEAN: return "boolean";
        case LuaType::THREAD: return "thread";
        default: throw std::logic_error("LuaType not implemented for to_string");
    }
}

inline std::string to_string(const FunctionType& function) {
    std::ostringstream oss;
    oss << "(";
    bool first = true;
    for (const auto& param : function.params) {
        if (!first) {
            oss << ",";
        }
        oss << ":" << to_string(param);
        first = false;
    }
    oss << "):" << to_string(*function.ret);
    return oss.str();
}

inline std::string to_string(const Type& type) {
    switch (type.get_tag()) {
        case Type::Tag::VOID: return "void";
        case Type::Tag::ANY: return "any";
        case Type::Tag::LUATYPE: return to_string(type.get_luatype());
        case Type::Tag::FUNCTION: return to_string(type.get_function());
        default: throw std::logic_error("Tag not implemented for to_string");
    }
}

struct AssignResult {
    bool yes = false;
    std::vector<std::string> messages;

    AssignResult() = default;
    AssignResult(bool b) : yes(b) {}
    AssignResult(bool b, std::string m) : yes(b), messages{std::move(m)} {}
};

inline std::string to_string(const AssignResult& ar) {
    std::string r;
    for (const auto& msg : ar.messages) {
        r = msg + "\n" + r;
    }
    return r;
}

inline AssignResult is_assignable(const Type& lval, const Type& rval);

inline AssignResult is_assignable(const LuaType& lval, const Type& rval) {
    switch (rval.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return lval == rval.get_luatype();
        default: return {false, "Cannot assign `" + to_string(rval) + "` to `" + to_string(lval) + "`"};
    }
}

inline AssignResult is_assignable(const FunctionType& lfunc, const Type& rval) {
    switch (rval.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::FUNCTION: {
            const auto& rfunc = rval.get_function();
            if (rfunc.params.size() < lfunc.params.size()) {
                return {false, "Cannot assign `" + to_string(rfunc) + "` to `" + to_string(lfunc) + "` (not enough parameters)"};
            }
            for (auto i = 0u; i < rfunc.params.size(); ++i) {
                AssignResult r;
                if (i < lfunc.params.size()) {
                    r = is_assignable(rfunc.params[i], lfunc.params[i]);
                } else {
                    r = is_assignable(rfunc.params[i], Type::make_luatype(LuaType::NIL));
                }
                if (!r.yes) {
                    r.messages.push_back("At parameter " + std::to_string(i));
                    r.messages.push_back("Cannot assign `" + to_string(rfunc) + "` to `" + to_string(lfunc) + "`");
                    return r;
                }
            }
            auto r = is_assignable(*lfunc.ret, *rfunc.ret);
            if (!r.yes) {
                r.messages.push_back("At return type");
            }
            return r;
        }
        default: return {false, "Cannot convert `" + to_string(rval) + "` to `" + to_string(lfunc) + "`"};
    }
}

inline AssignResult is_assignable(const Type& lval, const Type& rval) {
    switch (lval.get_tag()) {
        case Type::Tag::VOID: return {false, "Cannot assign `" + to_string(rval) + "` to `void`"};
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return is_assignable(lval.get_luatype(), rval);
        case Type::Tag::FUNCTION: return is_assignable(lval.get_function(), rval);
        default: throw std::logic_error("Tag not implemented for assignment");
    }
}

} // namespace typedlua
