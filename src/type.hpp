#pragma once

#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>

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

    static Type make_function(std::vector<Type> params, Type ret) {
        auto type = Type{};
        type.tag = Tag::FUNCTION;
        new (&type.function) FunctionType{std::move(params), std::make_unique<Type>(std::move(ret))};
        return type;
    }

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

} // namespace typedlua
