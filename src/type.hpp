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

struct TupleType {
    std::vector<Type> types;
    bool is_variadic = false;
};

class Type {
public:
    enum class Tag {
        VOID,
        ANY,
        LUATYPE,
        FUNCTION,
        TUPLE
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

    static Type make_tuple(std::vector<Type> types, bool is_variadic) {
        auto type = Type{};
        type.tag = Tag::TUPLE;
        new (&type.tuple) TupleType{std::move(types), is_variadic};
        return type;
    }

    const Tag& get_tag() const { return tag; }

    const LuaType& get_luatype() const { return luatype; }

    const FunctionType& get_function() const { return function; }

    const TupleType& get_tuple() const { return tuple; }

private:
    void destroy() {
        switch (tag) {
            case Tag::VOID: break;
            case Tag::ANY: break;
            case Tag::LUATYPE: break;
            case Tag::FUNCTION: std::destroy_at(&function); break;
            case Tag::TUPLE: std::destroy_at(&tuple); break;
            default: throw std::logic_error("Type tag not implemented");
        }
    }

    void assign_from(const Type& other) {
        switch (tag) {
            case Tag::VOID: break;
            case Tag::ANY: break;
            case Tag::LUATYPE: luatype = other.luatype; break;
            case Tag::FUNCTION: new (&function) FunctionType(other.function); break;
            case Tag::TUPLE: new (&tuple) TupleType(other.tuple); break;
            default: throw std::logic_error("Type tag not implemented");
        }
    }

    void assign_from(Type&& other) {
        switch (tag) {
            case Tag::FUNCTION: new (&function) FunctionType(std::move(other.function)); break;
            case Tag::TUPLE: new (&tuple) TupleType(std::move(other.tuple)); break;
            default: assign_from(other);
        }
    }

    Tag tag;
    union {
        LuaType luatype;
        FunctionType function;
        TupleType tuple;
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

inline std::string to_string(const TupleType& tuple) {
    std::ostringstream oss;
    oss << "[";
    bool first = true;
    for (const auto& type : tuple.types) {
        if (!first) {
            oss << ",";
        }
        oss << to_string(type);
        first = false;
    }
    if (tuple.is_variadic) {
        if (!first) {
            oss << ",";
        }
        oss << "...";
    }
    oss << "]";
    return oss.str();
}

inline std::string to_string(const Type& type) {
    switch (type.get_tag()) {
        case Type::Tag::VOID: return "void";
        case Type::Tag::ANY: return "any";
        case Type::Tag::LUATYPE: return to_string(type.get_luatype());
        case Type::Tag::FUNCTION: return to_string(type.get_function());
        case Type::Tag::TUPLE: return to_string(type.get_tuple());
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

template <typename T, typename U>
std::string cannot_assign(const T& lhs, const U& rhs) {
    return "Cannot assign`" + to_string(rhs) + "` to `" + to_string(lhs) + "`";
}

inline AssignResult is_assignable(const Type& lval, const Type& rval);

inline AssignResult is_assignable(const LuaType& lval, const Type& rval) {
    switch (rval.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: {
            if (lval != rval.get_luatype()) {
                return {false, cannot_assign(lval, rval)};
            }
            return true;
        }
        default: return {false, cannot_assign(rval, lval)};
    }
}

inline AssignResult is_assignable(const FunctionType& lfunc, const Type& rval) {
    switch (rval.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::FUNCTION: {
            const auto& rfunc = rval.get_function();
            if (rfunc.params.size() < lfunc.params.size()) {
                return {false, cannot_assign(rfunc, lfunc) + " (not enough parameters)"};
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
                    r.messages.push_back(cannot_assign(rfunc, lfunc));
                    return r;
                }
            }
            auto r = is_assignable(*lfunc.ret, *rfunc.ret);
            if (!r.yes) {
                r.messages.push_back("At return type");
            }
            return r;
        }
        default: return {false, cannot_assign(rval, lfunc)};
    }
}

inline AssignResult is_assignable(const Type& lval, const Type& rval) {
    switch (lval.get_tag()) {
        case Type::Tag::VOID: return {false, "Cannot assign `" + to_string(rval) + "` to `void`"};
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return is_assignable(lval.get_luatype(), rval);
        case Type::Tag::FUNCTION: return is_assignable(lval.get_function(), rval);
        case Type::Tag::TUPLE: return {false, "Cannot assign to tuple"};
        default: throw std::logic_error("Tag not implemented for assignment");
    }
}

inline AssignResult is_assignable(const std::vector<Type>& lhs, const std::vector<Type>& rhs, bool variadic) {
    if (!rhs.empty() && rhs.back().get_tag() == Type::Tag::TUPLE) {
        const auto& tup = rhs.back().get_tuple();
        auto newrhs = std::vector<Type>(rhs.begin(), rhs.end() - 1);
        newrhs.insert(newrhs.end(), tup.types.begin(), tup.types.end());
        return is_assignable(lhs, newrhs, tup.is_variadic);
    }

    const auto edge = std::min(lhs.size(), rhs.size());

    for (auto i = 0u; i < edge; ++i) {
        auto r = is_assignable(lhs[i], rhs[i]);
        if (!r.yes) {
            r.messages.push_back("At right-hand side item " + std::to_string(i + 1));
            return r;
        }
    }

    if (rhs.size() > lhs.size() || (rhs.size() == lhs.size() && variadic)) {
        return {true, "Too many expressions on right-hand side"};
    }

    if (lhs.size() > rhs.size() && !variadic) {
        for (auto i = rhs.size(); i < lhs.size(); ++i) {
            auto r = is_assignable(lhs[i], Type::make_luatype(LuaType::NIL));
            if (!r.yes) {
                r.messages.push_back("At left-hand side item " + std::to_string(i + 1));
                return r;
            }
        }
    }

    return true;
}

} // namespace typedlua
