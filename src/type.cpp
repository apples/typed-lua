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
            default: throw std::logic_error("Tag not implemented for to_string");
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
            case LuaType::STRING: oss << literal.string; break;
        }
        return oss.str();
    }

    std::string to_string(const FunctionType& function) {
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

} // namespace typedlua
