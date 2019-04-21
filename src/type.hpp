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
#include <variant>
#include <functional>

namespace typedlua {

std::string normalize_quotes(std::string_view s);

struct VoidType {};

struct AnyType {};

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
struct NameType;

struct FunctionType {
    std::vector<NameType> genparams;
    std::vector<int> nominals;
    std::vector<Type> params;
    std::unique_ptr<Type> ret;
    bool variadic = false;

    FunctionType() = default;
    FunctionType(std::vector<Type> params, std::unique_ptr<Type> ret, bool v) :
        params(std::move(params)),
        ret(std::move(ret)),
        variadic(v) {}
    FunctionType(std::vector<NameType> genparams, std::vector<int> nominals, std::vector<Type> params, std::unique_ptr<Type> ret, bool v) :
        genparams(std::move(genparams)),
        nominals(std::move(nominals)),
        params(std::move(params)),
        ret(std::move(ret)),
        variadic(v) {}
    FunctionType(FunctionType&&) = default;
    FunctionType(const FunctionType& other) :
        genparams(other.genparams),
        nominals(other.nominals),
        params(other.params),
        ret(std::make_unique<Type>(*other.ret)),
        variadic(other.variadic) {}
    
    FunctionType& operator=(FunctionType&&) = default;
    FunctionType& operator=(const FunctionType& other) {
        genparams = other.genparams;
        nominals = other.nominals;
        params = other.params;
        ret = std::make_unique<Type>(*other.ret);
        return *this;
    }
};

struct TupleType {
    std::vector<Type> types;
    bool is_variadic = false;
};

struct SumType {
    std::vector<Type> types;
};

struct ProductType {
    std::vector<Type> types;
};

using FieldMap = std::vector<NameType>;
struct TableType {
    std::vector<KeyValPair> indexes;
    FieldMap fields;
};

struct DeferredType {
    DeferredTypeCollection* collection;
    int id;
    std::vector<std::optional<Type>> args;
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

    NumberRep(const std::string& str) {
        {
            std::size_t pos;
            auto i = std::stoll(str, &pos);

            if (pos == str.size()) {
                is_integer = true;
                integer = i;
                return;
            }
        }

        {
            std::size_t pos;
            auto f = std::stod(str, &pos);

            if (pos == str.size()) {
                is_integer = false;
                floating = f;
                return;
            }
        }

        throw std::logic_error("Invalid number representation");
    }
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
    LiteralType(const NumberRep& n) : underlying_type(LuaType::NUMBER), number(n) {}
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

struct NominalType {
    DeferredType defer;
};

struct RequireType {
    std::unique_ptr<Type> basis;

    RequireType() = default;
    RequireType(std::unique_ptr<Type> basis) : basis(std::move(basis)) {}

    RequireType(const RequireType& other) :
        basis(std::make_unique<Type>(*other.basis))
    {}

    RequireType(RequireType&& other) = default;

    RequireType& operator=(const RequireType& other) {
        basis = std::make_unique<Type>(*other.basis);
        return *this;
    }

    RequireType& operator=(RequireType&& other) {
        std::swap(basis, other.basis);
        return *this;
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
        PRODUCT,
        TABLE,
        DEFERRED,
        LITERAL,
        NOMINAL,
        REQUIRE,
    };

    Type() = default;

    Type(LuaType lt) : types(lt) {}

    static Type make_any() {
        auto type = Type{};
        type.types = AnyType{};
        return type;
    }

    static Type make_luatype(LuaType lt) {
        auto type = Type();
        type.types = lt;
        return type;
    }

    static Type make_function(std::vector<Type> params, Type ret, bool variadic) {
        auto type = Type{};
        type.types = FunctionType{
            std::move(params),
            std::make_unique<Type>(std::move(ret)),
            variadic};
        return type;
    }

    static Type make_function(std::vector<NameType> genparams, std::vector<int> nominals, std::vector<Type> params, Type ret, bool variadic) {
        auto type = Type{};
        type.types = FunctionType{
            std::move(genparams),
            std::move(nominals),
            std::move(params),
            std::make_unique<Type>(std::move(ret)),
            variadic};
        return type;
    }

    static Type make_tuple(std::vector<Type> types, bool is_variadic) {
        auto type = Type{};
        type.types = TupleType{std::move(types), is_variadic};
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
        type.types = TableType{std::move(indexes), std::move(fields)};
        return type;
    }

    static Type make_deferred(DeferredTypeCollection& collection, int id) {
        auto type = Type{};
        type.types = DeferredType{&collection, id, {}};
        return type;
    }

    static Type make_deferred(DeferredTypeCollection& collection, int id, std::vector<std::optional<Type>> args) {
        auto type = Type{};
        type.types = DeferredType{&collection, id, std::move(args)};
        return type;
    }

    static Type make_literal(LiteralType literal) {
        auto type = Type{};
        type.types = LiteralType(std::move(literal));
        return type;
    }

    static Type make_nominal(DeferredTypeCollection& collection, int id) {
        auto type = Type{};
        type.types = NominalType{DeferredType{&collection, id}};
        return type;
    }

    static Type make_require(Type basis) {
        auto type = Type{};
        type.types = RequireType{std::make_unique<Type>(std::move(basis))};
        return type;
    }

    Tag get_tag() const { return static_cast<Tag>(types.index()); }

    const LuaType& get_luatype() const { return std::get<LuaType>(types); }

    const FunctionType& get_function() const { return std::get<FunctionType>(types); }

    const TupleType& get_tuple() const { return std::get<TupleType>(types); }

    const SumType& get_sum() const { return std::get<SumType>(types); }

    const ProductType& get_product() const { return std::get<ProductType>(types); }

    const TableType& get_table() const { return std::get<TableType>(types); }

    const DeferredType& get_deferred() const { return std::get<DeferredType>(types); }

    const LiteralType& get_literal() const { return std::get<LiteralType>(types); }

    const NominalType& get_nominal() const { return std::get<NominalType>(types); }

    const RequireType& get_require() const { return std::get<RequireType>(types); }

    friend Type operator|(const Type& lhs, const Type& rhs);
    
    friend Type operator&(const Type& lhs, const Type& rhs);

    friend Type operator-(const Type& lhs, const Type& rhs);

private:
    using Types = std::variant<
        VoidType,
        AnyType,
        LuaType,
        FunctionType,
        TupleType,
        SumType,
        ProductType,
        TableType,
        DeferredType,
        LiteralType,
        NominalType,
        RequireType>;

    Types types;
};

std::string to_string(const Type& type);
std::string to_string(const FunctionType& function);
std::string to_string(const TupleType& tuple);
std::string to_string(const SumType& sum);
std::string to_string(const ProductType& product);
std::string to_string(const KeyValPair& kvp);
std::string to_string(const TableType& table);
std::string to_string(const DeferredType& defer);
std::string to_string(const LuaType& luatype);
std::string to_string(const LiteralType& literal);
std::string to_string(const NominalType& nominal);
std::string to_string(const RequireType& require);

class DeferredTypeCollection {
public:
    int reserve(std::string name) {
        entries.emplace_back(Entry{{}, std::move(name)});
        return entries.size() - 1;
    }

    int reserve(std::string name, Type type) {
        entries.emplace_back(Entry{std::move(type), std::move(name), {}, false});
        return entries.size() - 1;
    }

    int reserve_narrow(std::string name) {
        entries.emplace_back(Entry{{}, std::move(name), {}, true});
        return entries.size() - 1;
    }

    const Type& get_type(int i) const {
        return entries[i].type;
    }

    const std::string& get_name(int i) const {
        return entries[i].name;
    }

    const std::vector<int>& get_nominals(int i) const {
        return entries[i].nominals;
    }

    void set(int i, Type t) {
        entries[i].type = std::move(t);
    }

    void set_nominals(int i, std::vector<int> nominals) {
        entries[i].nominals = std::move(nominals);
    }

    bool is_narrowing(int i) const {
        return entries[i].narrowing;
    }

private:
    struct Entry {
        Type type;
        std::string name;
        std::vector<int> nominals;
        bool narrowing = false;
    };

    std::vector<Entry> entries;
};

Type apply_genparams(
    const std::vector<std::optional<Type>>& genparams,
    const std::vector<int>& nominals,
    const std::function<Type(const std::string& name)>& get_package_type,
    const Type& type);

Type reduce_deferred(
    const DeferredType& defer,
    const std::function<Type(const std::string& name)>& get_package_type);

struct KeyValPair {
    Type key;
    Type val;
};

struct NameType {
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
inline AssignResult is_assignable(const Type& lhs, const ProductType& rproduct);
inline AssignResult is_assignable(const Type& lhs, const TupleType& rtuple);
inline AssignResult is_assignable(const Type& lhs, const TableType& rtable);
inline AssignResult is_assignable(const Type& lhs, const DeferredType& rdefer);
inline AssignResult is_assignable(const Type& lhs, const LiteralType& rliteral);
inline AssignResult is_assignable(const Type& lhs, const Type& rhs);

inline Type narrow_field(Type tabletype, const std::string& fieldname, const Type& fieldtype);
inline Type narrow_index(Type tabletype, const Type& keytype, const Type& valtype);

std::optional<Type> get_field_type(const Type& type, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables);

std::optional<Type> get_index_type(const Type& type, const Type& key, std::vector<std::string>& notes);


std::optional<Type> resolve_overload(
    const Type& type,
    const std::vector<Type>& args,
    std::vector<std::string>& notes,
    const std::function<Type(const std::string& name)>& get_package_type);

inline Type operator|(const Type& lhs, const Type& rhs) {
    if (is_assignable(lhs, rhs).yes) {
        return lhs;
    }

    auto rv = Type{};
    rv.types = SumType{};

    const auto lhs_size = lhs.get_tag() == Type::Tag::SUM ? lhs.get_sum().types.size() : 1;
    const auto rhs_size = rhs.get_tag() == Type::Tag::SUM ? rhs.get_sum().types.size() : 1;

    std::get<SumType>(rv.types).types.reserve(lhs_size + rhs_size);

    if (lhs.get_tag() == Type::Tag::SUM) {
        const auto& lhs_types = lhs.get_sum().types;
        std::get<SumType>(rv.types).types.insert(std::get<SumType>(rv.types).types.end(), lhs_types.begin(), lhs_types.end());
    } else {
        std::get<SumType>(rv.types).types.push_back(lhs);
    }

    if (rhs.get_tag() == Type::Tag::SUM) {
        const auto& rhs_types = rhs.get_sum().types;
        for (const auto& type : rhs_types) {
            if (!is_assignable(rv, type).yes) {
                std::get<SumType>(rv.types).types.push_back(type);
            }
        }
    } else {
        std::get<SumType>(rv.types).types.push_back(rhs);
    }

    return rv;
}

inline Type operator&(const Type& lhs, const Type& rhs) {
    if (is_assignable(lhs, rhs).yes) {
        return rhs;
    }

    if (is_assignable(rhs, lhs).yes) {
        return lhs;
    }

    if (lhs.get_tag() == Type::Tag::SUM) {
        auto rv = lhs;

        auto& sum = std::get<SumType>(rv.types);

        for (auto& type : sum.types) {
            type = type & rhs;
        }

        return rv;
    }

    if (rhs.get_tag() == Type::Tag::SUM) {
        auto rv = rhs;

        auto& sum = std::get<SumType>(rv.types);

        for (auto& type : sum.types) {
            type = lhs & type;
        }

        return rv;
    }

    auto rv = Type{};
    rv.types = ProductType{};

    auto& product = std::get<ProductType>(rv.types);

    const auto lhs_size = lhs.get_tag() == Type::Tag::PRODUCT ? lhs.get_product().types.size() : 1;
    const auto rhs_size = rhs.get_tag() == Type::Tag::PRODUCT ? rhs.get_product().types.size() : 1;

    product.types.reserve(lhs_size + rhs_size);

    if (lhs.get_tag() == Type::Tag::PRODUCT) {
        product.types = lhs.get_product().types;
    } else {
        product.types.push_back(lhs);
    }

    if (rhs.get_tag() == Type::Tag::PRODUCT) {
        const auto& rhs_types = rhs.get_product().types;
        product.types.insert(end(product.types), begin(rhs_types), end(rhs_types));
    } else {
        product.types.push_back(rhs);
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
        rv.types = SumType{std::move(result)};
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

    std::vector<NameType> newfields;

    bool found = false;

    for (auto field : table.fields) {
        if (field.name == fieldname) {
            field.type = field.type | fieldtype;
            found = true;
        }

        newfields.push_back(std::move(field));
    }

    if (!found) {
        newfields.push_back(NameType{fieldname, fieldtype});
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
    return is_assignable(reduce_deferred(ldefer, {}), rhs);
}

inline AssignResult is_divisible(const ProductType& product, const Type& divisor) {
    for (const auto& rtype : product.types) {
        auto r = is_assignable(divisor, rtype);
        if (r.yes) {
            return r;
        }
    }

    return false;
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

    auto lgenparams = std::vector<std::optional<Type>>{};
    auto rgenparams = std::vector<std::optional<Type>>{};

    lgenparams.reserve(lfunc.genparams.size());
    rgenparams.reserve(lfunc.genparams.size());
    
    for (const auto& gparam : lfunc.genparams) {
        lgenparams.push_back(gparam.type);
    }
    
    for (const auto& gparam : rfunc.genparams) {
        rgenparams.push_back(gparam.type);
    }

    for (auto i = 0u; i < rfunc.params.size(); ++i) {
        auto r = AssignResult{};

        if (i < lfunc.params.size()) {
            auto lparam = apply_genparams(lgenparams, lfunc.nominals, {}, lfunc.params[i]);
            auto rparam = apply_genparams(rgenparams, rfunc.nominals, {}, rfunc.params[i]);
            r = is_assignable(rparam, lparam);
        } else {
            auto rparam = apply_genparams(rgenparams, rfunc.nominals, {}, rfunc.params[i]);
            r = is_assignable(rparam, LuaType::NIL);
        }

        if (!r.yes) {
            r.messages.push_back("At parameter " + std::to_string(i));
            return r;
        }
    }

    auto lret = apply_genparams(lgenparams, lfunc.nominals, {}, *lfunc.ret);
    auto rret = apply_genparams(rgenparams, lfunc.nominals, {}, *rfunc.ret);

    auto r = is_assignable(lret, rret);

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

    return is_assignable(reduce_deferred(ldefer, {}), rdefer);
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

inline AssignResult is_assignable(const NominalType& lnominal, const NominalType& rnominal) {
    if (lnominal.defer.id != rnominal.defer.id) {
        return {false, cannot_assign(lnominal, rnominal)};
    }

    return true;
}

inline AssignResult is_assignable(const ProductType& lproduct, const ProductType& rproduct) {
    for (const auto& lhs : lproduct.types) {
        auto r = is_assignable(lhs, rproduct);
        if (!r.yes) {
            return r;
        }
    }

    return true;
}

inline AssignResult is_assignable(const Type& lhs, VoidType) {
    if (lhs.get_tag() == Type::Tag::VOID) {
        return true;
    } else {
        return {false, cannot_assign(lhs, Type{})};
    }
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
        default: return is_assignable(lhs, reduce_deferred(rdefer, {}));
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

inline AssignResult is_assignable(const Type& lhs, const NominalType& rnominal) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::NOMINAL: return is_assignable(lhs.get_nominal(), rnominal);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rnominal);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rnominal);
        default: return is_assignable(lhs, rnominal.defer);
    }
}

inline AssignResult is_assignable(const Type& lhs, const ProductType& rproduct) {
    switch (lhs.get_tag()) {
        case Type::Tag::ANY: return true;
        case Type::Tag::PRODUCT: return is_assignable(lhs.get_product(), rproduct);
        case Type::Tag::SUM: return is_assignable(lhs.get_sum(), rproduct);
        case Type::Tag::DEFERRED: return is_assignable(lhs.get_deferred(), rproduct);
        case Type::Tag::FUNCTION: return is_divisible(rproduct, lhs);
        default: return false;
    }
}

inline AssignResult is_assignable(const Type& lhs, const Type& rhs) {
    auto r = [&]() -> AssignResult {
        switch (rhs.get_tag()) {
            case Type::Tag::ANY: return true;
            case Type::Tag::VOID: return is_assignable(lhs, VoidType{});
            case Type::Tag::LUATYPE: return is_assignable(lhs, rhs.get_luatype());
            case Type::Tag::FUNCTION: return is_assignable(lhs, rhs.get_function());
            case Type::Tag::TUPLE: return is_assignable(lhs, rhs.get_tuple());
            case Type::Tag::SUM: return is_assignable(lhs, rhs.get_sum());
            case Type::Tag::TABLE: return is_assignable(lhs, rhs.get_table());
            case Type::Tag::DEFERRED: return is_assignable(lhs, rhs.get_deferred());
            case Type::Tag::LITERAL: return is_assignable(lhs, rhs.get_literal());
            case Type::Tag::NOMINAL: return is_assignable(lhs, rhs.get_nominal());
            case Type::Tag::PRODUCT: return is_assignable(lhs, rhs.get_product());
            default: return {false, "Tag not implemented for assignment"};
        }
    }();

    if (!r.yes) {
        r.messages.push_back(cannot_assign(lhs, rhs));
    }

    return r;
}

inline AssignResult check_param(
    const Type& param,
    const Type& arg,
    const std::vector<NameType>& genparams,
    const std::vector<int>& nominals,
    std::vector<std::optional<Type>>& genparams_inferred)
{
    switch (arg.get_tag()) {
        case Type::Tag::DEFERRED: {
            const auto& defer = arg.get_deferred();
            return check_param(param, reduce_deferred(defer, {}), genparams, nominals, genparams_inferred);
        }
    }

    switch (param.get_tag()) {
        case Type::Tag::NOMINAL: {
            auto id = param.get_nominal().defer.id;

            for (auto i = 0u; i < nominals.size(); ++i) {
                if (nominals[i] == id) {
                    auto& genparam = genparams[i];
                    auto& inferred = genparams_inferred[i];

                    if (inferred) {
                        return is_assignable(*inferred, arg);
                    } else {
                        auto r = check_param(genparam.type, arg, genparams, nominals, genparams_inferred);
                        if (r.yes) {
                            inferred = arg;
                        }
                        return r;
                    }
                }
            }

            return is_assignable(param, arg);
        }
        case Type::Tag::TABLE: {
            if (arg.get_tag() != Type::Tag::TABLE) {
                return {false, cannot_assign(param, arg)};
            }

            const auto& table = param.get_table();
            const auto& argtable = arg.get_table();

            for (const auto& index : table.indexes) {
                for (const auto& argindex : argtable.indexes) {
                    if (is_assignable(argindex.key, index.key).yes) {
                        auto r = check_param(index.val, argindex.val, genparams, nominals, genparams_inferred);

                        if (!r.yes) {
                            r.messages.push_back("When checking param table index `" + to_string(index.key) + "`");
                            return r;
                        }
                    }
                }
            }

            for (const auto& field : table.fields) {
                for (const auto& argfield : argtable.fields) {
                    if (field.name == argfield.name) {
                        auto r = check_param(field.type, argfield.type, genparams, nominals, genparams_inferred);

                        if (!r.yes) {
                            r.messages.push_back("When checking param table field `" + to_string(field.name) + "`");
                            return r;
                        }
                    }
                }
            }

            return true;
        } break;
        case Type::Tag::SUM: {
            const auto& sum = param.get_sum();

            for (const auto& type : sum.types) {
                auto r = check_param(type, arg, genparams, nominals, genparams_inferred);

                if (r.yes) {
                    return r;
                }
            }

            return {false, cannot_assign(param, arg)};
        } break;
        case Type::Tag::DEFERRED: {
            const auto& defer = param.get_deferred();
            const auto& type = reduce_deferred(defer, {});

            auto r = check_param(type, arg, genparams, nominals, genparams_inferred);

            if (!r.yes) {
                r.messages.push_back(cannot_assign(param, arg));
            }

            return r;
        } break;
        default: {
            auto reduced_param = apply_genparams(genparams_inferred, nominals, {}, param);
            return is_assignable(reduced_param, arg);
        } break;
    }
}

} // namespace typedlua
