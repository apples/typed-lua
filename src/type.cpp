#include "type.hpp"

#include <unordered_map>
#include <unordered_set>

namespace typedlua {

namespace { // static

struct TypePrinter {
    std::unordered_map<int, DeferredType> queue;
    std::unordered_set<int> seen;

    std::string to_string(const Type& type) {
        switch (type.get_tag()) {
            case Type::Tag::VOID: return "void";
            case Type::Tag::ANY: return "any";
            case Type::Tag::LUATYPE: return to_string(type.get_luatype());
            case Type::Tag::FUNCTION: return to_string(type.get_function());
            case Type::Tag::TUPLE: return to_string(type.get_tuple());
            case Type::Tag::SUM: return to_string(type.get_sum());
            case Type::Tag::TABLE: return to_string(type.get_table());
            case Type::Tag::DEFERRED: return to_string(type.get_deferred());
            case Type::Tag::LITERAL: return to_string(type.get_literal());
            case Type::Tag::NOMINAL: return to_string(type.get_nominal());
            case Type::Tag::REQUIRE: return to_string(type.get_require());
            default: throw std::logic_error("Tag " + std::to_string(static_cast<int>(type.get_tag())) + " not implemented for to_string");
        }
    }

    std::string to_string(const LuaType& luatype) {
        switch (luatype) {
            case LuaType::NIL: return "nil";
            case LuaType::NUMBER: return "number";
            case LuaType::STRING: return "string";
            case LuaType::BOOLEAN: return "boolean";
            case LuaType::THREAD: return "thread";
            default: throw std::logic_error("LuaType not implemented for to_string");
        }
    }

    std::string to_string(const LiteralType& literal) {
        std::ostringstream oss;
        switch (literal.underlying_type) {
            case LuaType::NIL: oss << "<nil literal>"; break;
            case LuaType::BOOLEAN: oss << (literal.boolean ? "true" : "false"); break;
            case LuaType::NUMBER:
                if (!literal.number.is_integer) {
                    oss << literal.number.floating;
                } else {
                    oss << literal.number.integer;
                }
                break;
            case LuaType::STRING: oss << "'" << literal.string << "'"; break;
        }
        return oss.str();
    }

    std::string to_string(const FunctionType& function) {
        std::ostringstream oss;
        
        if (!function.genparams.empty()) {
            bool first = true;
            oss << "<";
            for (const auto& gparam : function.genparams) {
                if (!first) {
                    oss << ",";
                }
                oss << gparam.name;
                oss << ":" << to_string(gparam.type);
                first = false;
            }
            oss << ">";
        }

        oss << "(";
        bool first = true;
        for (const auto& param : function.params) {
            if (!first) {
                oss << ",";
            }
            oss << ":" << to_string(param);
            first = false;
        }
        if (function.variadic) {
            if (!first) {
                oss << ",";
            }
            oss << "...";
        }
        oss << "):" << to_string(*function.ret);
        return oss.str();
    }

    std::string to_string(const TupleType& tuple) {
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

    std::string to_string(const SumType& sum) {
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

    std::string to_string(const KeyValPair& kvp) {
        return "[" + to_string(kvp.key) + "]:" + to_string(kvp.val);
    }

    std::string to_string(const TableType& table) {
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
        for (const auto& field : table.fields) {
            if (!first) {
                oss << ";";
            }
            oss << field.name << ":" << to_string(field.type);
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    std::string to_string(const DeferredType& defer) {
        if (seen.find(defer.id) == seen.end() && queue.find(defer.id) == queue.end()) {
            queue.emplace(defer.id, defer);
        }

        return defer.collection->get_name(defer.id);
    }

    std::string to_string(const NominalType& nominal) {
        return nominal.defer.collection->get_name(nominal.defer.id);
    }

    std::string to_string(const RequireType& require) {
        std::ostringstream oss;
        oss << "$require(";
        oss << to_string(*require.basis);
        oss << ")";
        return oss.str();
    }
};

template <typename T>
std::string to_string_impl(const T& type) {
    auto tp = TypePrinter{};

    auto result = tp.to_string(type);

    while (!tp.queue.empty()) {
        auto iter = tp.queue.begin();
        auto defer = iter->second;
        
        tp.seen.insert(iter->first);
        tp.queue.erase(iter);

        result += " with " + defer.collection->get_name(defer.id) + ":" + tp.to_string(defer.collection->get(defer.id));
    }

    return result;
}

} // static

std::string normalize_quotes(std::string_view value) {
    auto str = std::string{};
    auto escape_quotes = value[0] == '"';

    str.reserve(value.size() - 2);

    for (auto i = 1u; i < value.size() - 1; ++i) {
        auto c = value[i];
        if (escape_quotes) {
            switch (c) {
                case '\'':
                    str += "\\'";
                    break;
                case '\\':
                    ++i;
                    c = value[i];
                    switch (c) {
                        case '"':
                            str += '"';
                            break;
                        default:
                            str += '\\';
                            str += c;
                            break;
                    }
                    break;
                default:
                    str += c;
                    break;
            }
        } else {
            switch (c) {
                case '\\':
                    ++i;
                    c = value[i];
                    switch (c) {
                        case '"':
                            str += c;
                            break;
                        default:
                            str += '\\';
                            str += c;
                            break;
                    }
                    break;
                default:
                    str += c;
                    break;
            }
        }
    }

    return str;
}

std::string to_string(const Type& type) {
    return to_string_impl(type);
}

std::string to_string(const FunctionType& function) {
    return to_string_impl(function);
}

std::string to_string(const TupleType& tuple) {
    return to_string_impl(tuple);
}

std::string to_string(const SumType& sum) {
    return to_string_impl(sum);
}

std::string to_string(const KeyValPair& kvp) {
    return to_string_impl(kvp);
}

std::string to_string(const TableType& table) {
    return to_string_impl(table);
}

std::string to_string(const DeferredType& defer) {
    return to_string_impl(defer);
}

std::string to_string(const LuaType& luatype) {
    return to_string_impl(luatype);
}

std::string to_string(const LiteralType& literal) {
    return to_string_impl(literal);
}

std::string to_string(const NominalType& nominal) {
    return to_string_impl(nominal);
}

std::string to_string(const RequireType& require) {
    return to_string_impl(require);
}

Type apply_genparams(
    const std::vector<std::optional<Type>>& genparams,
    const std::vector<int>& nominals,
    const std::function<Type(const std::string& name)> get_package_type,
    const Type& type)
{
    if (genparams.empty()) {
        return type;
    }

    switch (type.get_tag()) {
        case Type::Tag::NOMINAL: {
            for (auto i = 0u; i < nominals.size(); ++i) {
                if (nominals[i] == type.get_nominal().defer.id) {
                    return genparams[i].value_or(Type::make_any());
                }
            }

            return type;
        }
        case Type::Tag::TABLE: {
            const auto& table = type.get_table();
            
            std::vector<KeyValPair> indexes;
            FieldMap fields;

            indexes.reserve(table.indexes.size());
            fields.reserve(table.fields.size());

            for (const auto& index : table.indexes) {
                auto key = apply_genparams(genparams, nominals, get_package_type, index.key);
                auto val = apply_genparams(genparams, nominals, get_package_type, index.val);

                indexes.push_back({std::move(key), std::move(val)});
            }

            for (const auto& field : table.fields) {
                auto fieldtype = apply_genparams(genparams, nominals, get_package_type, field.type);

                fields.push_back({field.name, fieldtype});
            }

            return Type::make_table(std::move(indexes), std::move(fields));
        }
        case Type::Tag::SUM: {
            const auto& sum = type.get_sum();

            std::optional<Type> rv;

            for (const auto& t : sum.types) {
                if (rv) {
                    rv = *rv | apply_genparams(genparams, nominals, get_package_type, t);
                } else {
                    rv = apply_genparams(genparams, nominals, get_package_type, t);
                }
            }

            return rv.value_or(Type::make_any());
        }
        case Type::Tag::TUPLE: {
            const auto& tuple = type.get_tuple();

            std::vector<Type> types;

            types.reserve(tuple.types.size());

            for (const auto& t : tuple.types) {
                types.push_back(apply_genparams(genparams, nominals, get_package_type, t));
            }

            return Type::make_tuple(std::move(types), tuple.is_variadic);
        }
        case Type::Tag::FUNCTION: {
            const auto& func = type.get_function();

            std::vector<NameType> gparams;
            std::vector<Type> params;
            Type ret;

            for (const auto& gparam : func.genparams) {
                gparams.push_back({gparam.name, apply_genparams(genparams, nominals, get_package_type, gparam.type)});
            }

            for (const auto& param : func.params) {
                params.push_back(apply_genparams(genparams, nominals, get_package_type, param));
            }

            ret = apply_genparams(genparams, nominals, get_package_type, *func.ret);

            return Type::make_function(
                std::move(gparams),
                func.nominals,
                std::move(params),
                std::move(ret),
                func.variadic);
        }
        case Type::Tag::REQUIRE: {
            const auto& require = type.get_require();

            auto inner_type = apply_genparams(genparams, nominals, get_package_type, *require.basis);

            if (get_package_type && inner_type.get_tag() == Type::Tag::LITERAL) {
                const auto& literal = inner_type.get_literal();
                if (literal.underlying_type == LuaType::STRING) {
                    return get_package_type(literal.string);
                }
            }

            return Type::make_any();
        }
        default:
            return type;
    }
}

namespace { // static

std::optional<Type> get_field_type(const LuaType& luatype, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables) {
    auto iter = luatype_metatables.find(luatype);

    if (iter != luatype_metatables.end()) {
        return get_field_type(iter->second, key, notes, luatype_metatables);
    }

    notes.push_back("LuaType " + to_string(luatype) + " has no metatable");

    return std::nullopt;
}

std::optional<Type> get_field_type(const TableType& table, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables) {
    for (const auto& field : table.fields) {
        if (field.name == key) {
            return field.type;
        }
    }

    return get_index_type(table, Type::make_luatype(LuaType::STRING), notes);
}

std::optional<Type> get_field_type(const SumType& sum, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables) {
    std::optional<Type> rv;

    for (const auto& type : sum.types) {
        auto t = get_field_type(type, key, notes, luatype_metatables);
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

std::optional<Type> get_field_type(const DeferredType& defer, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables) {
    auto r = get_field_type(defer.collection->get(defer.id), key, notes, luatype_metatables);
    if (!notes.empty()) {
        notes.push_back("In deferred type '" + defer.collection->get_name(defer.id) + "'");
    }
    return r;
}

} // static

std::optional<Type> get_field_type(const Type& type, const std::string& key, std::vector<std::string>& notes, const std::unordered_map<LuaType, Type>& luatype_metatables) {
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::LUATYPE: return get_field_type(type.get_luatype(), key, notes, luatype_metatables);
        case Type::Tag::SUM: return get_field_type(type.get_sum(), key, notes, luatype_metatables);
        case Type::Tag::TABLE: return get_field_type(type.get_table(), key, notes, luatype_metatables);
        case Type::Tag::DEFERRED: return get_field_type(type.get_deferred(), key, notes, luatype_metatables);
        case Type::Tag::LITERAL: return get_field_type(type.get_literal().underlying_type, key, notes, luatype_metatables);
        default:
            notes.push_back("Type `" + to_string(type) + "` has no fields");
            return std::nullopt;
    }
}

namespace { // static

std::optional<Type> get_index_type(const TableType& table, const Type& key, std::vector<std::string>& notes) {
    for (const auto& index : table.indexes) {
        if (is_assignable(index.key, key).yes) {
            return index.val;
        }
    }

    return std::nullopt;
}

std::optional<Type> get_index_type(const SumType& sum, const Type& key, std::vector<std::string>& notes) {
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

std::optional<Type> get_index_type(const DeferredType& defer, const Type& key, std::vector<std::string>& notes) {
    return get_index_type(defer.collection->get(defer.id), key, notes);
}

} // static

std::optional<Type> get_index_type(const Type& type, const Type& key, std::vector<std::string>& notes) {
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::SUM: return get_index_type(type.get_sum(), key, notes);
        case Type::Tag::TABLE: return get_index_type(type.get_table(), key, notes);
        case Type::Tag::DEFERRED: return get_index_type(type.get_deferred(), key, notes);
        case Type::Tag::NOMINAL: return get_index_type(type.get_nominal().defer, key, notes);
        default:
            notes.push_back("Type `" + to_string(type) + "` has no indexes");
            return std::nullopt;
    }
}

namespace { // static

std::optional<Type> resolve_overload(
    const FunctionType& func,
    const std::vector<Type>& args,
    std::vector<std::string>& notes,
    const std::function<Type(const std::string& name)>& get_package_type)
{
    if (args.size() > func.params.size() && !func.variadic) {
        notes.emplace_back("Too many arguments for non-variadic function");
        return std::nullopt;
    } else {
        auto nils = 0;

        if (args.size() < func.params.size()) {
            nils = func.params.size() - args.size();
        }

        auto genparams_inferred = std::vector<std::optional<Type>>{};

        genparams_inferred.resize(func.genparams.size());

        const auto sz = std::min(args.size() + nils, func.params.size());

        auto nil = Type::make_luatype(LuaType::NIL);

        for (auto i = 0u; i < sz; ++i) {
            const auto& argstype = i < args.size() ? args[i] : nil;
            const auto& lhstype = func.params[i];

            auto r = check_param(lhstype, argstype, func.genparams, func.nominals, genparams_inferred);

            if (!r.yes) {
                r.messages.push_back("Invalid parameter " + std::to_string(i));
                notes.emplace_back(to_string(r));
                return std::nullopt;
            } else if (!r.messages.empty()) {
                notes.emplace_back(to_string(r));
            }
        }

        return apply_genparams(genparams_inferred, func.nominals, get_package_type, *func.ret);
    }
}

std::optional<Type> resolve_overload(
    const ProductType& product,
    const std::vector<Type>& args,
    std::vector<std::string>& notes,
    const std::function<Type(const std::string& name)>& get_package_type)
{
    auto all_notes = std::vector<std::string>{};

    for (const auto& type : product.types) {
        auto cur_notes = std::vector<std::string>{};
        
        auto result = resolve_overload(type, args, cur_notes, get_package_type);

        if (result) {
            notes.insert(
                end(notes),
                make_move_iterator(begin(cur_notes)),
                make_move_iterator(end(cur_notes)));
            
            return result;
        }

        all_notes.insert(
            end(all_notes),
            make_move_iterator(begin(cur_notes)),
            make_move_iterator(end(cur_notes)));
    }

    notes.insert(
        end(notes),
        make_move_iterator(begin(all_notes)),
        make_move_iterator(end(all_notes)));
    
    return std::nullopt;
}

std::optional<Type> resolve_overload(
    const DeferredType& defer,
    const std::vector<Type>& args,
    std::vector<std::string>& notes,
    const std::function<Type(const std::string& name)>& get_package_type)
{
    return resolve_overload(defer.collection->get(defer.id), args, notes, get_package_type);
}

} // static

std::optional<Type> resolve_overload(
    const Type& type,
    const std::vector<Type>& args,
    std::vector<std::string>& notes,
    const std::function<Type(const std::string& name)>& get_package_type)
{
    switch (type.get_tag()) {
        case Type::Tag::ANY: return Type::make_any();
        case Type::Tag::FUNCTION: return resolve_overload(type.get_function(), args, notes, get_package_type);
        case Type::Tag::PRODUCT: return resolve_overload(type.get_product(), args, notes, get_package_type);
        case Type::Tag::DEFERRED: return resolve_overload(type.get_deferred(), args, notes, get_package_type);
        default:
            notes.push_back("Type `" + to_string(type) + "` cannot be called");
            return std::nullopt;
    }
}

} // namespace typedlua
