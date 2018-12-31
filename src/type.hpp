#pragma once

#include <stdexcept>
#include <utility>
#include <memory>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <optional>

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
struct FieldDecl;

struct FunctionType {
    std::vector<Type> params;
    std::unique_ptr<Type> ret;
    bool variadic = false;

    FunctionType() = default;
    FunctionType(std::vector<Type> params, std::unique_ptr<Type> ret, bool v) :
        params(std::move(params)),
        ret(std::move(ret)),
        variadic(v) {}
    FunctionType(FunctionType&&) = default;
    FunctionType(const FunctionType& other) :
        params(other.params),
        ret(std::make_unique<Type>(*other.ret)),
        variadic(other.variadic) {}
};

struct TupleType {
    std::vector<Type> types;
    bool is_variadic = false;
};

struct SumType {
    std::vector<Type> types;
};

using FieldMap = std::vector<FieldDecl>;
struct TableType {
    std::vector<KeyValPair> indexes;
    FieldMap fields;
};

struct DeferredType {
    DeferredTypeCollection* collection;
    int id;
};

struct NumberRep {
    bool is_integer;
    union {
        double floating;
        std::int64_t integer;
    };

    NumberRep() = default;
    NumberRep(double f) : is_integer(false), floating(f) {}
    NumberRep(std::int64_t i) : is_integer(true), integer(i) {}
};

inline bool operator==(const NumberRep& lhs, const NumberRep& rhs) {
    if (lhs.is_integer != rhs.is_integer) {
        return false;
    }

    if (!lhs.is_integer) {
        return lhs.floating == rhs.floating;
    } else {
        return lhs.integer == rhs.integer;
    }
}

struct LiteralType {
    LuaType underlying_type;
    union {
        bool boolean;
        NumberRep number;
        std::string string;
    };

    LiteralType() : underlying_type(LuaType::NIL) {}
    LiteralType(bool b) : underlying_type(LuaType::BOOLEAN), boolean(b) {}
    LiteralType(double n) : underlying_type(LuaType::NUMBER), number(n) {}
    LiteralType(std::int64_t n) : underlying_type(LuaType::NUMBER), number(n) {}
    LiteralType(std::string s) : underlying_type(LuaType::STRING), string(std::move(s)) {}

    LiteralType(LiteralType&& other) :
        underlying_type(std::exchange(other.underlying_type, LuaType::NIL))
    {
        switch (underlying_type) {
            case LuaType::BOOLEAN: boolean = other.boolean; break;
            case LuaType::NUMBER: number = other.number; break;
            case LuaType::STRING:
                new (&string) std::string(std::move(other.string));
                break;
        }
    }

    LiteralType(const LiteralType& other) :
        underlying_type(other.underlying_type)
    {
        switch (underlying_type) {
            case LuaType::BOOLEAN: boolean = other.boolean; break;
            case LuaType::NUMBER: number = other.number; break;
            case LuaType::STRING:
                new (&string) std::string(other.string);
                break;
        }
    }

    LiteralType& operator=(LiteralType&& other) {
        switch (underlying_type) {
            case LuaType::STRING:
                std::destroy_at(&string);
                break;
        }

        underlying_type = std::exchange(other.underlying_type, LuaType::NIL);
        switch (underlying_type) {
            case LuaType::BOOLEAN: boolean = other.boolean; break;
            case LuaType::NUMBER: number = other.number; break;
            case LuaType::STRING:
                new (&string) std::string(std::move(other.string));
                break;
        }

        return *this;
    }

    LiteralType& operator=(const LiteralType& other) {
        switch (underlying_type) {
            case LuaType::STRING:
                std::destroy_at(&string);
                break;
        }
        
        underlying_type = other.underlying_type;
        switch (underlying_type) {
            case LuaType::BOOLEAN: boolean = other.boolean; break;
            case LuaType::NUMBER: number = other.number; break;
            case LuaType::STRING:
                new (&string) std::string(other.string);
                break;
        }

        return *this;
    }

    ~LiteralType() {
        switch (underlying_type) {
            case LuaType::STRING:
                std::destroy_at(&string);
                break;
        }
    }
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
        DEFERRED,
        LITERAL
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

    static Type make_function(std::vector<Type> params, Type ret, bool variadic) {
        auto type = Type{};
        type.tag = Tag::FUNCTION;
        new (&type.function) FunctionType{std::move(params), std::make_unique<Type>(std::move(ret)), variadic};
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

    static Type make_table(std::vector<KeyValPair> indexes, FieldMap fields) {
        auto type = Type{};
        type.tag = Tag::TABLE;
        new (&type.table) TableType{std::move(indexes), std::move(fields)};
        return type;
    }

    static Type make_deferred(DeferredTypeCollection& collection, int id) {
        auto type = Type{};
        type.tag = Tag::DEFERRED;
        new (&type.deferred) DeferredType{&collection, id};
        return type;
    }

    static Type make_literal(LiteralType literal) {
        auto type = Type{};
        type.tag = Tag::LITERAL;
        new (&type.literal) LiteralType(std::move(literal));
        return type;
    }

    const Tag& get_tag() const { return tag; }

    const LuaType& get_luatype() const { return luatype; }

    const FunctionType& get_function() const { return function; }

    const TupleType& get_tuple() const { return tuple; }

    const SumType& get_sum() const { return sum; }

    const TableType& get_table() const { return table; }

    const DeferredType& get_deferred() const { return deferred; }

    const LiteralType& get_literal() const { return literal; }

    friend Type operator|(const Type& lhs, const Type& rhs);

    friend Type operator-(const Type& lhs, const Type& rhs);

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
            case Tag::LITERAL: std::destroy_at(&literal); break;
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
            case Tag::LITERAL: new (&literal) LiteralType(other.literal); break;
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
            case Tag::LITERAL: new (&literal) LiteralType(std::move(other.literal)); break;
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
        LiteralType literal;
    };
};

std::string to_string(const Type& type);
std::string to_string(const FunctionType& function);
std::string to_string(const TupleType& tuple);
std::string to_string(const SumType& sum);
std::string to_string(const KeyValPair& kvp);
std::string to_string(const TableType& table);
std::string to_string(const DeferredType& defer);
std::string to_string(const LuaType& luatype);
std::string to_string(const LiteralType& literal);

class DeferredTypeCollection {
public:
    int reserve(std::string name) {
        entries.emplace_back(Entry{{}, std::move(name)});
        return entries.size() - 1;
    }

    int reserve_narrow(std::string name) {
        entries.emplace_back(Entry{{}, std::move(name), true});
        return entries.size() - 1;
    }

    const Type& get(int i) const {
        return entries[i].type;
    }

    const std::string& get_name(int i) const {
        return entries[i].name;
    }

    void set(int i, Type t) {
        entries[i].type = std::move(t);
    }

    bool is_narrowing(int i) const {
        return entries[i].narrowing;
    }

private:
    struct Entry {
        Type type;
        std::string name;
        bool narrowing = true;
    };

    std::vector<Entry> entries;
};

struct KeyValPair {
    Type key;
    Type val;
};

struct FieldDecl {
    std::string name;
    Type type;
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
inline AssignResult is_assignable(const LiteralType& lliteral, const LiteralType& rliteral);
inline AssignResult is_assignable(const Type& lhs, const LuaType& rlua);
inline AssignResult is_assignable(const Type& lhs, const FunctionType& rfunc);
inline AssignResult is_assignable(const Type& lhs, const SumType& rsum);
inline AssignResult is_assignable(const Type& lhs, const TupleType& rtuple);
inline AssignResult is_assignable(const Type& lhs, const TableType& rtable);
inline AssignResult is_assignable(const Type& lhs, const DeferredType& rdefer);
inline AssignResult is_assignable(const Type& lhs, const LiteralType& rliteral);
inline AssignResult is_assignable(const Type& lhs, const Type& rhs);

inline Type narrow_field(Type tabletype, const std::string& fieldname, const Type& fieldtype);
inline Type narrow_index(Type tabletype, const Type& keytype, const Type& valtype);

inline std::optional<Type> get_field_type(const TableType& table, const std::string& key, std::vector<std::string>& notes);
inline std::optional<Type> get_field_type(const SumType& sum, const std::string& key, std::vector<std::string>& notes);
inline std::optional<Type> get_field_type(const DeferredType& defer, const std::string& key, std::vector<std::string>& notes);
inline std::optional<Type> get_field_type(const Type& type, const std::string& key, std::vector<std::string>& notes);

inline std::optional<Type> get_index_type(const TableType& table, const Type& key, std::vector<std::string>& notes);
inline std::optional<Type> get_index_type(const SumType& sum, const Type& key, std::vector<std::string>& notes);
inline std::optional<Type> get_index_type(const DeferredType& defer, const Type& key, std::vector<std::string>& notes);
inline std::optional<Type> get_index_type(const Type& type, const Type& key, std::vector<std::string>& notes);

inline std::optional<Type> get_return_type(const FunctionType& func, std::vector<std::string>& notes);
inline std::optional<Type> get_return_type(const Type& type, std::vector<std::string>& notes);

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

inline Type operator-(const Type& lhs, const Type& rhs) {

    if (lhs.get_tag() == Type::Tag::SUM) {
        std::vector<Type> result;

        for (const auto& type : lhs.get_sum().types) {
            auto reduced = type - rhs;
            if (reduced.get_tag() != Type::Tag::VOID) {
                result.push_back(std::move(reduced));
            }
        }

        if (result.size() == 0) {
            return Type{};
        }

        if (result.size() == 1) {
            return std::move(result[0]);
        }

        auto rv = Type{};
        rv.tag = Type::Tag::SUM;
        new (&rv.sum) SumType{std::move(result)};
        return rv;
    }

    if (rhs.get_tag() == Type::Tag::SUM) {
        auto result = lhs;

        for (const auto& type : rhs.get_sum().types) {
            result = result - rhs;
        }

        return result;
    }

    switch (rhs.get_tag()) {
        case Type::Tag::LITERAL:
            switch (lhs.get_tag()) {
                case Type::Tag::LUATYPE: {
                    const auto& literal = rhs.get_literal();

                    if (lhs.get_luatype() == literal.underlying_type) {
                        switch (literal.underlying_type) {
                            case LuaType::BOOLEAN: return Type::make_literal(!literal.boolean);
                            default: return lhs;
                        }
                    }
                } break;
                case Type::Tag::LITERAL: {
                    const auto& lliteral = lhs.get_literal();
                    const auto& rliteral = rhs.get_literal();

                    auto are_same = [&]{
                        if (lliteral.underlying_type == rliteral.underlying_type) {
                            switch (lliteral.underlying_type) {
                                case LuaType::BOOLEAN:
                                    return lliteral.boolean == rliteral.boolean;
                                case LuaType::NUMBER:
                                    return lliteral.number == rliteral.number;
                                case LuaType::STRING:
                                    return lliteral.string == rliteral.string;
                            }
                        }

                        return false;
                    };

                    if (are_same()) {
                        return Type{};
                    } else {
                        return lhs;
                    }
                }
            }
            break;
    }

    return lhs;
}

inline Type narrow_field(Type tabletype, const std::string& fieldname, const Type& fieldtype) {
    if (tabletype.get_tag() != Type::Tag::TABLE) {
        throw std::logic_error("Cannot narrow table field of type `" + to_string(tabletype) + "`");
    }

    const auto& table = tabletype.get_table();

    std::vector<FieldDecl> newfields;

    bool found = false;

    for (auto field : table.fields) {
        if (field.name == fieldname) {
            field.type = field.type | fieldtype;
            found = true;
        }

        newfields.push_back(std::move(field));
    }

    if (!found) {
        newfields.push_back(FieldDecl{fieldname, fieldtype});
    }

    return Type::make_table(table.indexes, std::move(newfields));
}

inline Type narrow_index(Type tabletype, const Type& keytype, const Type& valtype) {
    if (tabletype.get_tag() != Type::Tag::TABLE) {
        throw std::logic_error("Cannot narrow table field of type `" + to_string(tabletype) + "`");
    }

    const auto& table = tabletype.get_table();

    std::vector<KeyValPair> newindexes;

    bool found = false;

    for (auto index : table.indexes) {
        if (is_assignable(index.key, keytype).yes) {
            index.val = index.val | valtype;
            found = true;
        }

        newindexes.push_back(std::move(index));
    }

    if (!found) {
        newindexes.push_back({keytype, valtype});
    }

    return Type::make_table(std::move(newindexes), table.fields);
}

inline std::optional<Type> get_field_type(const TableType& table, const std::string& key, std::vector<std::string>& notes) {
    for (const auto& field : table.fields) {
        if (field.name == key) {
            return field.type;
        }
    }

    return get_index_type(table, Type::make_luatype(LuaType::STRING), notes);
}

inline std::optional<Type> get_field_type(const SumType& sum, const std::string& key, std::vector<std::string>& notes) {
    std::optional<Type> rv;

    for (const auto& type : sum.types) {
        auto t = get_field_type(type, key, notes);
        if (t) {
            if (!rv) {
                rv = std::move(t);
            } else {
                rv = *rv | *t;
            }
        } else {
            notes.push_back("Cannot find field '" + key + "' in `" + to_string(type) + "`");
        }
    }

    return rv;
}

inline std::optional<Type> get_field_type(const DeferredType& defer, const std::string& key, std::vector<std::string>& notes) {
    auto r = get_field_type(defer.collection->get(defer.id), key, notes);
    if (!notes.empty()) {
        notes.push_back("In deferred type '" + defer.collection->get_name(defer.id) + "'");
    }
    return r;
}

inline std::optional<Type> get_field_type(const Type& type, const std::string& key, std::vector<std::string>& notes) {
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::SUM: return get_field_type(type.get_sum(), key, notes);
        case Type::Tag::TABLE: return get_field_type(type.get_table(), key, notes);
        case Type::Tag::DEFERRED: return get_field_type(type.get_deferred(), key, notes);
        default:
            notes.push_back("Type `" + to_string(type) + "` has no fields");
            return std::nullopt;
    }
}

inline std::optional<Type> get_index_type(const TableType& table, const Type& key, std::vector<std::string>& notes) {
    for (const auto& index : table.indexes) {
        if (is_assignable(index.key, key).yes) {
            return index.val;
        }
    }

    return std::nullopt;
}

inline std::optional<Type> get_index_type(const SumType& sum, const Type& key, std::vector<std::string>& notes) {
    std::optional<Type> rv;

    for (const auto& type : sum.types) {
        auto t = get_index_type(type, key, notes);
        if (t) {
            if (!rv) {
                rv = std::move(t);
            } else {
                rv = *rv | *t;
            }
        } else {
            notes.push_back("Cannot find index `" + to_string(key) + "` in `" + to_string(type) + "`");
        }
    }

    return rv;
}

inline std::optional<Type> get_index_type(const DeferredType& defer, const Type& key, std::vector<std::string>& notes) {
    return get_index_type(defer.collection->get(defer.id), key, notes);
}

inline std::optional<Type> get_index_type(const Type& type, const Type& key, std::vector<std::string>& notes) {
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::SUM: return get_index_type(type.get_sum(), key, notes);
        case Type::Tag::TABLE: return get_index_type(type.get_table(), key, notes);
        case Type::Tag::DEFERRED: return get_index_type(type.get_deferred(), key, notes);
        default:
            notes.push_back("Type `" + to_string(type) + "` has no indexes");
            return std::nullopt;
    }
}

inline std::optional<Type> get_return_type(const FunctionType& func, std::vector<std::string>& notes) {
    if (func.ret) {
        return *func.ret;
    } else {
        notes.push_back("Function `" + to_string(func) + "` has no return type");
        return std::nullopt;
    }
}

inline std::optional<Type> get_return_type(const SumType& sum, std::vector<std::string>& notes) {
    std::optional<Type> rv;

    for (const auto& type : sum.types) {
        auto t = get_return_type(type, notes);
        if (t) {
            if (!rv) {
                rv = std::move(t);
            } else {
                rv = *rv | *t;
            }
        } else {
            notes.push_back("Cannot call `" + to_string(sum) + "`");
        }
    }

    return rv;
}

inline std::optional<Type> get_return_type(const DeferredType& defer, std::vector<std::string>& notes) {
    return get_return_type(defer.collection->get(defer.id), notes);
}

inline std::optional<Type> get_return_type(const Type& type, std::vector<std::string>& notes) {
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::SUM: return get_return_type(type.get_sum(), notes);
        case Type::Tag::FUNCTION: return get_return_type(type.get_function(), notes);
        case Type::Tag::DEFERRED: return get_return_type(type.get_deferred(), notes);
        default:
            notes.push_back("Type `" + to_string(type) + "` cannot be called");
            return std::nullopt;
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
        return false;
    }
    return true;
}

inline AssignResult is_assignable(const Type& lhs, const LuaType& rlua) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::LUATYPE: return is_assignable(lhs.get_luatype(), rlua);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rlua);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rlua);
        default: return false;
    }
}

inline AssignResult is_assignable(const FunctionType& lfunc, const FunctionType& rfunc) {
    if (rfunc.params.size() < lfunc.params.size()) {
        return {false, "Not enough parameters."};
    }

    for (auto i = 0u; i < rfunc.params.size(); ++i) {
        auto r = AssignResult{};

        if (i < lfunc.params.size()) {
            r = is_assignable(rfunc.params[i], lfunc.params[i]);
        } else {
            r = is_assignable(rfunc.params[i], LuaType::NIL);
        }

        if (!r.yes) {
            r.messages.push_back("At parameter " + std::to_string(i));
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
        for (const auto& rindex : rtable.indexes) {
            if (is_assignable(rindex.key, lindex.key).yes) {
                auto r = is_assignable(lindex.val, rindex.val);
                if (!r.yes) {
                    r.messages.push_back("When checking index `" + to_string(lindex) + "` against `" + to_string(rindex) + "`");
                    return r;
                }
            }
        }

        if (is_assignable(lindex.key, LuaType::STRING).yes) {
            for (const auto& rfield : rtable.fields) {
                auto r = is_assignable(lindex.val, rfield.type);
                if (!r.yes) {
                    r.messages.push_back("When checking table index `" + to_string(lindex) + "` against field `" + rfield.name + "`");
                    return r;
                }
            }
        }
    }

    for (const auto& lfield : ltable.fields) {
        bool found = false;

        for (const auto& rfield : rtable.fields) {
            if (lfield.name == rfield.name) {
                auto r = is_assignable(lfield.type, rfield.type);
                if (!r.yes) {
                    r.messages.push_back("At field '" + lfield.name + "`");
                    return r;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            auto r = is_assignable(lfield.type, LuaType::NIL);
            if (!r.yes) {
                r.messages.push_back("Field '" + lfield.name + "' is missing in right-hand side");
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

inline AssignResult is_assignable(const LiteralType& lliteral, const LiteralType& rliteral) {
    if (lliteral.underlying_type == rliteral.underlying_type) {
        switch (lliteral.underlying_type) {
            case LuaType::BOOLEAN:
                return lliteral.boolean == rliteral.boolean;
            case LuaType::NUMBER:
                return lliteral.number == rliteral.number;
            case LuaType::STRING:
                return lliteral.string == rliteral.string;
            default:
                throw std::logic_error("Unsupported literal type");
        }
    }

    return false;
}

inline AssignResult is_assignable(const Type& lhs, const FunctionType& rfunc) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::FUNCTION: return is_assignable(lhs.get_function(), rfunc);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rfunc);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rfunc);
        default: return false;
    }
}

inline AssignResult is_assignable(const Type& lhs, const SumType& rsum) {
    for (const auto& type : rsum.types) {
        auto r = is_assignable(lhs, type);
        if (!r.yes) {
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
        default: return false;
    }
}

inline AssignResult is_assignable(const Type& lhs, const TableType& rtable) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::TABLE: return is_assignable(lhs.get_table(), rtable);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rtable);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rtable);
        default: return false;
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

inline AssignResult is_assignable(const Type& lhs, const LiteralType& rliteral) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::LITERAL: return is_assignable(lhs.get_literal(), rliteral);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rliteral);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rliteral);
        default: return is_assignable(lhs, rliteral.underlying_type);
    }
}

inline AssignResult is_assignable(const Type& lhs, const Type& rhs) {
    auto r = [&]() -> AssignResult {
        switch (rhs.get_tag()) {
            case Type::Tag::VOID: return {false, "Cannot assign `void` to `" + to_string(lhs) + "`"};
            case Type::Tag::ANY: return true;
            case Type::Tag::LUATYPE: return is_assignable(lhs, rhs.get_luatype());
            case Type::Tag::FUNCTION: return is_assignable(lhs, rhs.get_function());
            case Type::Tag::TUPLE: return is_assignable(lhs, rhs.get_tuple());
            case Type::Tag::SUM: return is_assignable(lhs, rhs.get_sum());
            case Type::Tag::TABLE: return is_assignable(lhs, rhs.get_table());
            case Type::Tag::DEFERRED: return is_assignable(lhs, rhs.get_deferred());
            case Type::Tag::LITERAL: return is_assignable(lhs, rhs.get_literal());
            default: throw std::logic_error("Tag not implemented for assignment");
        }
    }();

    if (!r.yes) {
        r.messages.push_back(cannot_assign(lhs, rhs));
    }

    return r;
}

} // namespace typedlua
