#pragma once

#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>

namespace typedlua {

enum class LuaType {
    NIL,
    NUMBER,
    STRING,
    BOOLEAN,
    THREAD
};

class Type;
class DeferredTypeCollection;
struct KeyValPair;

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

struct SumType {
    std::vector<Type> types;
};

struct TableType {
    std::vector<KeyValPair> indexes;
};

struct DeferredType {
    const DeferredTypeCollection* collection;
    int id;
};

class Type {
public:
    enum class Tag {
        VOID,
        ANY,
        LUATYPE,
        FUNCTION,
        TUPLE,
        SUM,
        TABLE,
        DEFERRED
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

    static Type make_reduced_tuple(std::vector<Type> types) {
        if (types.size() == 1) {
            return std::move(types[0]);
        } else {
            return make_tuple(std::move(types), false);
        }
    }

    static Type make_table(std::vector<KeyValPair> indexes) {
        auto type = Type{};
        type.tag = Tag::TABLE;
        new (&type.table) TableType{std::move(indexes)};
        return type;
    }

    static Type make_deferred(const DeferredTypeCollection& collection, int id) {
        auto type = Type{};
        type.tag = Tag::DEFERRED;
        new (&type.deferred) DeferredType{&collection, id};
        return type;
    }

    const Tag& get_tag() const { return tag; }

    const LuaType& get_luatype() const { return luatype; }

    const FunctionType& get_function() const { return function; }

    const TupleType& get_tuple() const { return tuple; }

    const SumType& get_sum() const { return sum; }

    const TableType& get_table() const { return table; }

    const DeferredType& get_deferred() const { return deferred; }

    friend Type operator|(const Type& lhs, const Type& rhs);

private:
    void destroy() {
        switch (tag) {
            case Tag::VOID: break;
            case Tag::ANY: break;
            case Tag::LUATYPE: break;
            case Tag::FUNCTION: std::destroy_at(&function); break;
            case Tag::TUPLE: std::destroy_at(&tuple); break;
            case Tag::SUM: std::destroy_at(&sum); break;
            case Tag::TABLE: std::destroy_at(&table); break;
            case Tag::DEFERRED: std::destroy_at(&deferred); break;
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
            case Tag::SUM: new (&sum) SumType(other.sum); break;
            case Tag::TABLE: new (&table) TableType(other.table); break;
            case Tag::DEFERRED: new (&deferred) DeferredType(other.deferred); break;
            default: throw std::logic_error("Type tag not implemented");
        }
    }

    void assign_from(Type&& other) {
        switch (tag) {
            case Tag::FUNCTION: new (&function) FunctionType(std::move(other.function)); break;
            case Tag::TUPLE: new (&tuple) TupleType(std::move(other.tuple)); break;
            case Tag::SUM: new (&sum) SumType(std::move(other.sum)); break;
            case Tag::TABLE: new (&table) TableType(std::move(other.table)); break;
            case Tag::DEFERRED: new (&deferred) DeferredType(std::move(other.deferred)); break;
            default: assign_from(other);
        }
    }

    Tag tag;
    union {
        LuaType luatype;
        FunctionType function;
        TupleType tuple;
        SumType sum;
        TableType table;
        DeferredType deferred;
    };
};

inline std::string to_string(const Type& type);

class DeferredTypeCollection {
public:
    int reserve() {
        types.emplace_back();
        return types.size() - 1;
    }

    const Type& get(int i) const {
        return types[i];
    }

    void set(int i, Type t) {
        types[i] = std::move(t);
    }

private:
    std::vector<Type> types;
};

struct KeyValPair {
    Type key;
    Type val;
};

struct AssignResult {
    bool yes = false;
    std::vector<std::string> messages;

    AssignResult() = default;
    AssignResult(bool b) : yes(b) {}
    AssignResult(bool b, std::string m) : yes(b), messages{std::move(m)} {}
};

template <typename RHS>
AssignResult is_assignable(const SumType& lsum, const RHS& rhs);
template <typename RHS>
AssignResult is_assignable(const DeferredType& ldefer, const RHS& rhs);

inline AssignResult is_assignable(const LuaType& llua, const LuaType& rlua);
inline AssignResult is_assignable(const FunctionType& lfunc, const FunctionType& rfunc);
inline AssignResult is_assignable(const TupleType& ltuple, const TupleType& rtuple);
inline AssignResult is_assignable(const TableType& ltable, const TableType& rtable);
inline AssignResult is_assignable(const DeferredType& ldefer, const DeferredType& rdefer);
inline AssignResult is_assignable(const Type& lhs, const LuaType& rlua);
inline AssignResult is_assignable(const Type& lhs, const FunctionType& rfunc);
inline AssignResult is_assignable(const Type& lhs, const SumType& rsum);
inline AssignResult is_assignable(const Type& lhs, const TupleType& rtuple);
inline AssignResult is_assignable(const Type& lhs, const TableType& rtable);
inline AssignResult is_assignable(const Type& lhs, const DeferredType& rdefer);
inline AssignResult is_assignable(const Type& lhs, const Type& rhs);

inline Type operator|(const Type& lhs, const Type& rhs) {
    if (is_assignable(lhs, rhs).yes) {
        return lhs;
    }

    auto rv = Type{};
    rv.tag = Type::Tag::SUM;
    new (&rv.sum) SumType{};

    const auto lhs_size = lhs.get_tag() == Type::Tag::SUM ? lhs.get_sum().types.size() : 1;
    const auto rhs_size = rhs.get_tag() == Type::Tag::SUM ? rhs.get_sum().types.size() : 1;

    rv.sum.types.reserve(lhs_size + rhs_size);

    if (lhs.get_tag() == Type::Tag::SUM) {
        const auto& lhs_types = lhs.get_sum().types;
        rv.sum.types.insert(rv.sum.types.end(), lhs_types.begin(), lhs_types.end());
    } else {
        rv.sum.types.push_back(lhs);
    }

    if (rhs.get_tag() == Type::Tag::SUM) {
        const auto& rhs_types = rhs.get_sum().types;
        for (const auto& type : rhs_types) {
            if (!is_assignable(rv, type).yes) {
                rv.sum.types.push_back(type);
            }
        }
    } else {
        rv.sum.types.push_back(rhs);
    }

    return rv;
}

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

inline std::string to_string(const SumType& sum) {
    std::ostringstream oss;
    bool first = true;
    for (const auto& type : sum.types) {
        if (!first) {
            oss << "|";
        }
        oss << to_string(type);
        first = false;
    }
    return oss.str();
}

inline std::string to_string(const KeyValPair& kvp) {
    return "[" + to_string(kvp.key) + "]:" + to_string(kvp.val);
}

inline std::string to_string(const TableType& table) {
    std::ostringstream oss;
    oss << "{";
    bool first = true;
    for (const auto& index : table.indexes) {
        if (!first) {
            oss << ";";
        }
        oss << to_string(index);
        first = false;
    }
    return oss.str();
}

inline std::string to_string(const DeferredType& defer) {
    std::ostringstream oss;
    oss << "<deferred #" << defer.id << ">";
    return oss.str();
}

inline std::string to_string(const Type& type) {
    switch (type.get_tag()) {
        case Type::Tag::VOID: return "void";
        case Type::Tag::ANY: return "any";
        case Type::Tag::LUATYPE: return to_string(type.get_luatype());
        case Type::Tag::FUNCTION: return to_string(type.get_function());
        case Type::Tag::TUPLE: return to_string(type.get_tuple());
        case Type::Tag::SUM: return to_string(type.get_sum());
        case Type::Tag::TABLE: return to_string(type.get_table());
        case Type::Tag::DEFERRED: return to_string(type.get_deferred());
        default: throw std::logic_error("Tag not implemented for to_string");
    }
}

inline std::string to_string(const AssignResult& ar) {
    std::string r;
    for (const auto& msg : ar.messages) {
        r = msg + "\n" + r;
    }
    return r;
}

template <typename T, typename U>
std::string cannot_assign(const T& lhs, const U& rhs) {
    return "Cannot assign `" + to_string(rhs) + "` to `" + to_string(lhs) + "`";
}

template <typename RHS>
AssignResult is_assignable(const SumType& lsum, const RHS& rhs) {
    for (const auto& type : lsum.types) {
        auto r = is_assignable(type, rhs);
        if (r.yes) return r;
    }
    return {false, cannot_assign(lsum, rhs)};
}

template <typename RHS>
AssignResult is_assignable(const DeferredType& ldefer, const RHS& rhs) {
    return is_assignable(ldefer.collection->get(ldefer.id), rhs);
}

inline AssignResult is_assignable(const LuaType& llua, const LuaType& rlua) {
    if (llua != rlua) {
        return {false, cannot_assign(llua, rlua)};
    }
    return true;
}

inline AssignResult is_assignable(const Type& lhs, const LuaType& rlua) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return is_assignable(lhs.get_luatype(), rlua);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rlua);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rlua);
        default: return {false, cannot_assign(lhs, rlua)};
    }
}

inline AssignResult is_assignable(const FunctionType& lfunc, const FunctionType& rfunc) {
    if (rfunc.params.size() < lfunc.params.size()) {
        return {false, cannot_assign(lfunc, rfunc) + " (not enough parameters)"};
    }

    for (auto i = 0u; i < rfunc.params.size(); ++i) {
        AssignResult r;

        if (i < lfunc.params.size()) {
            r = is_assignable(rfunc.params[i], lfunc.params[i]);
        } else {
            r = is_assignable(rfunc.params[i], LuaType::NIL);
        }

        if (!r.yes) {
            r.messages.push_back("At parameter " + std::to_string(i));
            r.messages.push_back(cannot_assign(lfunc, rfunc));
            return r;
        }
    }

    auto r = is_assignable(*lfunc.ret, *rfunc.ret);

    if (!r.yes) {
        r.messages.push_back("At return type");
    }

    return r;
}

inline AssignResult is_assignable(const TupleType& ltuple, const TupleType& rtuple) {
    const auto& lhs = ltuple.types;
    const auto& rhs = rtuple.types;

    if (!rhs.empty() && rhs.back().get_tag() == Type::Tag::TUPLE) {
        const auto& tup = rhs.back().get_tuple();
        auto newrhs = std::vector<Type>(rhs.begin(), rhs.end() - 1);
        newrhs.insert(newrhs.end(), tup.types.begin(), tup.types.end());
        return is_assignable(ltuple, TupleType{newrhs, tup.is_variadic});
    }

    const auto edge = std::min(lhs.size(), rhs.size());

    for (auto i = 0u; i < edge; ++i) {
        auto r = is_assignable(lhs[i], rhs[i]);
        if (!r.yes) {
            r.messages.push_back("At item " + std::to_string(i + 1));
            return r;
        }
    }

    if (lhs.size() > rhs.size() && !rtuple.is_variadic) {
        for (auto i = edge; i < lhs.size(); ++i) {
            auto r = is_assignable(lhs[i], LuaType::NIL);
            if (!r.yes) {
                r.messages.push_back("Not enough values on right-hand side");
                return r;
            }
        }
    }

    if (lhs.size() < rhs.size() && !ltuple.is_variadic) {
        return {false, "Too many values on right-hand size"};
    }

    return true;
}

inline AssignResult is_assignable(const TableType& ltable, const TableType& rtable) {
    for (const auto& lindex : ltable.indexes) {
        bool found = false;
        for (const auto& rindex : rtable.indexes) {
            if (is_assignable(rindex.key, lindex.key).yes) {
                auto r = is_assignable(lindex.val, rindex.val);
                if (!r.yes) {
                    r.messages.push_back("When checking index `" + to_string(lindex) + "` against `" + to_string(rindex) + "`");
                    return r;
                }
                found = true;
            }
        }
        if (!found) {
            auto r = is_assignable(lindex.val, LuaType::NIL);
            if (!r.yes) {
                r.messages.push_back("At table index `" + to_string(lindex) + "`");
                return r;
            }
        }
    }
    return true;
}

inline AssignResult is_assignable(const DeferredType& ldefer, const DeferredType& rdefer) {
    if (ldefer.collection == rdefer.collection && ldefer.id == rdefer.id) {
        return true;
    }
    return is_assignable(ldefer.collection->get(ldefer.id), rdefer);
}

inline AssignResult is_assignable(const Type& lhs, const FunctionType& rfunc) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::FUNCTION: return is_assignable(lhs.get_function(), rfunc);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rfunc);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rfunc);
        default: return {false, cannot_assign(lhs, rfunc)};
    }
}

inline AssignResult is_assignable(const Type& lhs, const SumType& rsum) {
    for (const auto& type : rsum.types) {
        auto r = is_assignable(lhs, type);
        if (!r.yes) {
            r.messages.push_back(cannot_assign(lhs, rsum));
            return r;
        }
    }
    return true;
}

inline AssignResult is_assignable(const Type& lhs, const TupleType& rtuple) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::TUPLE: return is_assignable(lhs.get_tuple(), rtuple);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rtuple);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rtuple);
        default: return {false, cannot_assign(lhs, rtuple)};
    }
}

inline AssignResult is_assignable(const Type& lhs, const TableType& rtable) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::TABLE: return is_assignable(lhs.get_table(), rtable);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rtable);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rtable);
        default: return {false, cannot_assign(lhs, rtable)};
    }
}

inline AssignResult is_assignable(const Type& lhs, const DeferredType& rdefer) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rdefer);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rdefer);
        default: return is_assignable(lhs, rdefer.collection->get(rdefer.id));
    }
}

inline AssignResult is_assignable(const Type& lhs, const Type& rhs) {
    switch (rhs.get_tag()) {
        case Type::Tag::VOID: return {false, "Cannot assign `void` to `" + to_string(lhs) + "`"};
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return is_assignable(lhs, rhs.get_luatype());
        case Type::Tag::FUNCTION: return is_assignable(lhs, rhs.get_function());
        case Type::Tag::TUPLE: return is_assignable(lhs, rhs.get_tuple());
        case Type::Tag::SUM: return is_assignable(lhs, rhs.get_sum());
        case Type::Tag::TABLE: return is_assignable(lhs, rhs.get_table());
        case Type::Tag::DEFERRED: return is_assignable(lhs, rhs.get_deferred());
        default: throw std::logic_error("Tag not implemented for assignment");
    }
}

} // namespace typedlua
