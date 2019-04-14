#include "node.hpp"

namespace typedlua::ast {

void Node::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    // do nothing
}

void Node::dump(std::ostream& out) const {
    out << "--[[NOT IMPLEMENTED]]";
}

void NExpr::check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const {
    check(parent_scope, errors);
}

void NBlock::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto this_scope = Scope(&parent_scope);
    for (const auto& child : children) {
        child->check(this_scope, errors);
    }
}

void NBlock::dump(std::ostream& out) const {
    if (scoped) out << "do\n";
    for (const auto& child : children) {
        out << *child << "\n";
    }
    if (scoped) out << "end";
}

void NType::dump(std::ostream& out) const {
    throw std::logic_error("Types cannot be emitted");
}

NNameDecl::NNameDecl(std::string name)
    : name(std::move(name)) {}
NNameDecl::NNameDecl(std::string name, std::unique_ptr<NType> type)
    : name(std::move(name)), type(std::move(type)) {}

void NNameDecl::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (type) type->check(parent_scope, errors);
}

void NNameDecl::dump(std::ostream& out) const {
    out << name;
}

Type NNameDecl::get_type(const Scope& scope) const {
    if (type) {
        return type->get_type(scope);
    } else {
        return Type::make_any();
    }
}

NTypeName::NTypeName(std::string name)
    : name(std::move(name)) {}

void NTypeName::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (!parent_scope.get_type(name)) {
        errors.emplace_back("Type `" + name + "` not in scope", location);
    }
}

Type NTypeName::get_type(const Scope& scope) const {
    if (auto type = scope.get_type(name)) {
        return *type;
    } else {
        return Type::make_any();
    }
}

NTypeFunctionParam::NTypeFunctionParam(std::string name, std::unique_ptr<NType> type)
    : name(std::move(name)), type(std::move(type)) {}

void NTypeFunctionParam::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    type->check(parent_scope, errors);
}

void NTypeFunctionParam::dump(std::ostream& out) const {
    out << name << ":" << *type;
}

NTypeFunction::NTypeFunction(std::vector<NTypeFunctionParam> p, std::unique_ptr<NType> r, bool v)
    : params(std::move(p)), ret(std::move(r)), is_variadic(v) {}

void NTypeFunction::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto scope = Scope(&parent_scope);
    auto& deferred = scope.get_deferred_types();

    std::vector<NameType> genparams;
    std::vector<int> nominals;
    std::vector<Type> paramtypes;

    for (auto i = 0u; i < generic_params.size(); ++i) {
        const auto& gparam = generic_params[i];
        gparam.check(scope, errors);
        auto id = deferred.reserve(gparam.name);
        scope.add_type(gparam.name, Type::make_nominal(deferred, id));
        genparams.push_back({gparam.name, gparam.get_type(scope)});
        nominals.push_back(id);
    }

    for (const auto& param : params) {
        param.check(scope, errors);
        paramtypes.push_back(param.type->get_type(scope));
    }

    ret->check(scope, errors);

    cached_type = Type::make_function(std::move(genparams), nominals, std::move(paramtypes), ret->get_type(scope), is_variadic);
}

Type NTypeFunction::get_type(const Scope& parent_scope) const {
    return cached_type;
}

NTypeTuple::NTypeTuple(std::vector<NTypeFunctionParam> p, bool v)
    : params(std::move(p)), variadic(v) {}

void NTypeTuple::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& param : params) {
        param.check(parent_scope, errors);
    }
}

Type NTypeTuple::get_type(const Scope& scope) const {
    std::vector<Type> paramtypes;
    for (const auto& param : params) {
        paramtypes.push_back(param.type->get_type(scope));
    }
    return Type::make_tuple(std::move(paramtypes), variadic);
}

NTypeSum::NTypeSum(std::unique_ptr<NType> l, std::unique_ptr<NType> r)
    : lhs(std::move(l)), rhs(std::move(r)) {}

void NTypeSum::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    lhs->check(parent_scope, errors);
    rhs->check(parent_scope, errors);
}

Type NTypeSum::get_type(const Scope& scope) const {
    return lhs->get_type(scope) | rhs->get_type(scope);
}

NIndex::NIndex(std::unique_ptr<NType> k, std::unique_ptr<NType> v)
    : key(std::move(k)), val(std::move(v)) {}

void NIndex::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    key->check(parent_scope, errors);
    val->check(parent_scope, errors);

    auto ktype = key->get_type(parent_scope);

    if (is_assignable(ktype, LuaType::NIL).yes) {
        errors.emplace_back("Key type must not be compatible with `nil`", key->location);
    }
}

void NIndex::dump(std::ostream& out) const {
    out << "[" << *key << "]:" << *val;
}

KeyValPair NIndex::get_kvp(const Scope& scope) const {
    return {key->get_type(scope), val->get_type(scope)};
}

void NIndexList::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& index : indexes) {
        index->check(parent_scope, errors);
    }
}

void NIndexList::dump(std::ostream& out) const {
    bool first = true;
    for (const auto& index : indexes) {
        if (!first) {
            out << ";";
        }
        out << *index;
        first = false;
    }
}

std::vector<KeyValPair> NIndexList::get_types(const Scope& scope) const {
    std::vector<KeyValPair> rv;
    for (const auto& index : indexes) {
        rv.push_back(index->get_kvp(scope));
    }
    return rv;
}

NFieldDecl::NFieldDecl(std::string n, std::unique_ptr<NType> t)
    : name(std::move(n)), type(std::move(t)) {}

void NFieldDecl::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    type->check(parent_scope, errors);
}

void NFieldDecl::dump(std::ostream& out) const {
    out << name << ":" << *type;
}

void NFieldDeclList::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& field : fields) {
        field->check(parent_scope, errors);

        auto iter = std::find_if(cached_fields.begin(), cached_fields.end(), [&](NameType& fd) { return fd.name == field->name; });

        if (iter != cached_fields.end()) {
            errors.emplace_back("Duplicate table key '" + field->name + "'", location);
            iter->type = iter->type | field->type->get_type(parent_scope);
        } else {
            cached_fields.push_back({field->name, field->type->get_type(parent_scope)});
        }
    }
}

void NFieldDeclList::dump(std::ostream& out) const {
    bool first = true;
    for (const auto& field : fields) {
        if (!first) {
            out << ";";
        }
        out << *field;
        first = false;
    }
}

FieldMap NFieldDeclList::get_types(const Scope& scope) const {
    return cached_fields;
}

NTypeTable::NTypeTable(std::unique_ptr<NIndexList> i, std::unique_ptr<NFieldDeclList> f)
    : indexlist(std::move(i)), fieldlist(std::move(f)) {}

void NTypeTable::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (indexlist) indexlist->check(parent_scope, errors);
    if (fieldlist) fieldlist->check(parent_scope, errors);
}

Type NTypeTable::get_type(const Scope& scope) const {
    std::vector<KeyValPair> indexes;
    FieldMap fields;

    if (indexlist) indexes = indexlist->get_types(scope);
    if (fieldlist) fields = fieldlist->get_types(scope);

    return Type::make_table(std::move(indexes), std::move(fields));
}

NTypeLiteralBoolean::NTypeLiteralBoolean(bool v)
    : value(v) {}

Type NTypeLiteralBoolean::get_type(const Scope& scope) const {
    return Type::make_literal(value);
}

NTypeLiteralNumber::NTypeLiteralNumber(const std::string& s)
    : value(s) {}

Type NTypeLiteralNumber::get_type(const Scope& scope) const {
    return Type::make_literal(value);
}

NTypeLiteralString::NTypeLiteralString(std::string_view s)
    : value(normalize_quotes(s)) {}

Type NTypeLiteralString::get_type(const Scope& scope) const {
    return Type::make_literal(value);
}

NTypeRequire::NTypeRequire(std::unique_ptr<NType> t)
    : type(std::move(t)) {}

void NTypeRequire::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    type->check(parent_scope, errors);
}

Type NTypeRequire::get_type(const Scope& scope) const {
    return Type::make_require(type->get_type(scope));
}

NInterface::NInterface(std::string n, std::unique_ptr<NType> t)
    : name(std::move(n)), type(std::move(t)) {}

void NInterface::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (auto oldtype = parent_scope.get_type(name)) {
        errors.emplace_back(CompileError::Severity::WARNING, "Interface `" + name + "` shadows existing type", location);
    }

    auto& deferred = parent_scope.get_deferred_types();
    const auto deferred_id = deferred.reserve(name);

    parent_scope.add_type(name, Type::make_deferred(deferred, deferred_id));
    type->check(parent_scope, errors);

    deferred.set(deferred_id, type->get_type(parent_scope));
}

void NInterface::dump(std::ostream& out) const {
    // do nothing
}

NIdent::NIdent(std::string v)
    : name(std::move(v)) {}

void NIdent::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (!parent_scope.get_type_of(name)) {
        fail_common(parent_scope, errors);
    }
}

void NIdent::check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const {
    if (auto type = parent_scope.get_type_of(name)) {
        if (type->get_tag() == Type::Tag::DEFERRED) {
            const auto& defer = type->get_deferred();

            if (!defer.collection->is_narrowing(defer.id)) {
                return;
            }

            const auto& current_type = defer.collection->get(defer.id);

            auto narrowed_type = current_type | expected;

            defer.collection->set(defer.id, std::move(narrowed_type));
        }
    } else {
        fail_common(parent_scope, errors);
    }
}

void NIdent::dump(std::ostream& out) const {
    out << name;
}

Type NIdent::get_type(const Scope& scope) const {
    if (auto type = scope.get_type_of(name)) {
        return *type;
    } else {
        return Type::make_any();
    }
}

void NIdent::fail_common(Scope& parent_scope, std::vector<CompileError>& errors) const {
    errors.emplace_back("Name `" + name + "` is not in scope", location);
    // Prevent further type errors for this name
    parent_scope.add_name(name, Type::make_any());
}

NSubscript::NSubscript(std::unique_ptr<NExpr> p, std::unique_ptr<NExpr> s)
    : prefix(std::move(p)), subscript(std::move(s)) {}

void NSubscript::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);
    subscript->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);
    auto keytype = subscript->get_type(parent_scope);

    check_common(prefixtype, keytype, parent_scope, errors);
}

void NSubscript::check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);
    subscript->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);
    auto keytype = subscript->get_type(parent_scope);

    if (prefixtype.get_tag() == Type::Tag::DEFERRED) {
        const auto& defer = prefixtype.get_deferred();

        if (!defer.collection->is_narrowing(defer.id)) {
            return check_common(prefixtype, keytype, parent_scope, errors);
        }

        const auto& current_type = defer.collection->get(defer.id);

        if (current_type.get_tag() != Type::Tag::TABLE) {
            return check_common(prefixtype, keytype, parent_scope, errors);
        }

        auto narrowed_type = narrow_index(current_type, keytype, expected);

        defer.collection->set(defer.id, std::move(narrowed_type));
    } else {
        return check_common(prefixtype, keytype, parent_scope, errors);
    }
}

void NSubscript::dump(std::ostream& out) const {
    out << *prefix << "[" << *subscript << "]";
}

Type NSubscript::get_type(const Scope& scope) const {
    if (cached_type) {
        return *cached_type;
    } else {
        return Type::make_any();
    }
}
void NSubscript::check_common(const Type& prefixtype, const Type& keytype, Scope& parent_scope, std::vector<CompileError>& errors) const {
    std::vector<std::string> notes;

    auto result = get_index_type(prefixtype, keytype, notes);

    if (!result) {
        notes.push_back("Could not find index `" + to_string(keytype) + "` in `" + to_string(prefixtype) + "`");
    }

    if (!notes.empty()) {
        std::string msg;
        for (const auto& note : notes) {
            msg = note + "\n" + msg;
        }
        errors.emplace_back(msg, location);
    }

    cached_type = std::move(result);
}

NTableAccess::NTableAccess(std::unique_ptr<NExpr> p, std::string n)
    : prefix(std::move(p)), name(std::move(n)) {}

void NTableAccess::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);

    return check_common(prefixtype, parent_scope, errors);
}

void NTableAccess::check_expect(Scope& parent_scope, const Type& expected, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);

    if (prefixtype.get_tag() == Type::Tag::DEFERRED) {
        const auto& defer = prefixtype.get_deferred();

        if (!defer.collection->is_narrowing(defer.id)) {
            return check_common(prefixtype, parent_scope, errors);
        }

        const auto& current_type = defer.collection->get(defer.id);

        if (current_type.get_tag() != Type::Tag::TABLE) {
            return check_common(prefixtype, parent_scope, errors);
        }

        auto narrowed_type = narrow_field(current_type, name, expected);

        defer.collection->set(defer.id, std::move(narrowed_type));
    } else {
        return check_common(prefixtype, parent_scope, errors);
    }
}

void NTableAccess::dump(std::ostream& out) const {
    out << *prefix << "." << name;
}

Type NTableAccess::get_type(const Scope& scope) const {
    if (cached_type) {
        return *cached_type;
    } else {
        return Type::make_any();
    }
}

void NTableAccess::check_common(const Type& prefixtype, Scope& parent_scope, std::vector<CompileError>& errors) const {
    std::vector<std::string> notes;

    auto result = get_field_type(prefixtype, name, notes, parent_scope.get_luatype_metatable_map());

    if (!result) {
        notes.push_back("Could not find field '" + name + "' in `" + to_string(prefixtype) + "`");
    }

    if (!notes.empty()) {
        std::string msg;
        for (const auto& note : notes) {
            msg = note + "\n" + msg;
        }
        errors.emplace_back(msg, location);
    }

    cached_type = std::move(result);
}

void NArgSeq::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& arg : args) {
        arg->check(parent_scope, errors);
    }
}

void NArgSeq::dump(std::ostream& out) const {
    bool first = true;
    for (const auto& arg : args) {
        if (!first) {
            out << ",";
        }
        out << *arg;
        first = false;
    }
}

NFunctionCall::NFunctionCall(std::unique_ptr<NExpr> p, std::unique_ptr<NArgSeq> a)
    : prefix(std::move(p)), args(std::move(a)) {}

void NFunctionCall::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);
    if (args) args->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);

    switch (prefixtype.get_tag()) {
        case Type::Tag::ANY: break;
        case Type::Tag::FUNCTION: {
            const auto& func = prefixtype.get_function();

            auto rhs = std::vector<Type>{};

            // Convert args into a semi-tuple
            if (args) {
                rhs.reserve(args->args.size());

                for (const auto& expr : args->args) {
                    rhs.push_back(expr->get_type(parent_scope));
                }
            }

            if (rhs.size() > func.params.size() && !func.variadic) {
                errors.emplace_back("Too many arguments for non-variadic function", location);
            } else {
                if (rhs.size() < func.params.size()) {
                    std::fill_n(std::back_inserter(rhs), func.params.size() - rhs.size(), Type::make_luatype(LuaType::NIL));
                }

                std::vector<std::optional<Type>> genparams_inferred;

                genparams_inferred.resize(func.genparams.size());

                const auto sz = std::min(rhs.size(), func.params.size());

                for (auto i = 0u; i < sz; ++i) {
                    const auto& rhstype = rhs[i];
                    const auto& lhstype = func.params[i];

                    auto r = check_param(lhstype, rhstype, func.genparams, func.nominals, genparams_inferred);

                    if (!r.yes) {
                        r.messages.push_back("Invalid parameter " + std::to_string(i));
                        errors.emplace_back(to_string(r), location);
                    } else if (!r.messages.empty()) {
                        errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
                    }
                }

                cached_rettype = apply_genparams(genparams_inferred, func.nominals, parent_scope.get_get_package_type(), *func.ret);
            }
        } break;
        default: errors.emplace_back("Cannot call non-function type `" + to_string(prefixtype) + "`", location); break;
    }
}

void NFunctionCall::dump(std::ostream& out) const {
    out << *prefix << "(";
    if (args) out << *args;
    out << ")";
}

Type NFunctionCall::get_type(const Scope& scope) const {
    return cached_rettype.value_or(Type::make_any());
}

NFunctionSelfCall::NFunctionSelfCall(std::unique_ptr<NExpr> p, std::string n, std::unique_ptr<NArgSeq> a)
    : prefix(std::move(p)), name(std::move(n)), args(std::move(a)) {}

void NFunctionSelfCall::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    prefix->check(parent_scope, errors);
    if (args) args->check(parent_scope, errors);

    auto prefixtype = prefix->get_type(parent_scope);

    std::vector<std::string> notes;

    auto functype = get_field_type(prefixtype, name, notes, parent_scope.get_luatype_metatable_map());

    if (!functype) {
        notes.push_back("Could not find method '" + name + "' in type `" + to_string(prefixtype) + "`");
    } else {
        cached_rettype = get_return_type(*functype, notes);

        switch ((*functype).get_tag()) {
            case Type::Tag::ANY: break;
            case Type::Tag::FUNCTION: {
                const auto& func = (*functype).get_function();

                auto rhs = std::vector<Type>{};

                // Convert args into a semi-tuple
                if (args) {
                    rhs.reserve(args->args.size() + 1);

                    rhs.push_back(prefixtype);

                    for (const auto& expr : args->args) {
                        rhs.push_back(expr->get_type(parent_scope));
                    }
                } else {
                    rhs.push_back(prefixtype);
                }

                if (rhs.size() > func.params.size() && !func.variadic) {
                    errors.emplace_back("Too many arguments for non-variadic function", location);
                } else {
                    if (rhs.size() < func.params.size()) {
                        std::fill_n(std::back_inserter(rhs), func.params.size() - rhs.size(), Type::make_luatype(LuaType::NIL));
                    }

                    std::vector<std::optional<Type>> genparams_inferred;

                    genparams_inferred.resize(func.genparams.size());

                    const auto sz = std::min(rhs.size(), func.params.size());

                    for (auto i = 0u; i < sz; ++i) {
                        const auto& rhstype = rhs[i];
                        const auto& lhstype = func.params[i];

                        auto r = check_param(lhstype, rhstype, func.genparams, func.nominals, genparams_inferred);

                        if (!r.yes) {
                            r.messages.push_back("Invalid parameter " + std::to_string(i));
                            errors.emplace_back(to_string(r), location);
                        } else if (!r.messages.empty()) {
                            errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
                        }
                    }

                    cached_rettype = apply_genparams(genparams_inferred, func.nominals, parent_scope.get_get_package_type(), *func.ret);
                }
            } break;
            default: errors.emplace_back("Cannot call non-function type `" + to_string(*functype) + "`", location); break;
        }
    }

    if (!notes.empty()) {
        std::string msg;
        for (const auto& note : notes) {
            msg = note + "\n" + msg;
        }
        errors.emplace_back(msg, location);
    }
}

void NFunctionSelfCall::dump(std::ostream& out) const {
    out << *prefix << ":" << name << "(";
    if (args) out << *args;
    out << ")";
}

Type NFunctionSelfCall::get_type(const Scope& scope) const {
    if (cached_rettype) {
        return *cached_rettype;
    } else {
        return Type::make_any();
    }
}

NNumberLiteral::NNumberLiteral(std::string v)
    : value(std::move(v)) {}

void NNumberLiteral::dump(std::ostream& out) const {
    out << value;
}

Type NNumberLiteral::get_type(const Scope& scope) const {
    return Type::make_literal(NumberRep(value));
}

NAssignment::NAssignment(std::vector<std::unique_ptr<NExpr>> v, std::vector<std::unique_ptr<NExpr>> e)
    : vars(std::move(v)), exprs(std::move(e)) {}

void NAssignment::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    std::vector<Type> lhs;
    std::vector<Type> rhs;

    lhs.reserve(vars.size());
    rhs.reserve(exprs.size());

    for (const auto& expr : exprs) {
        expr->check(parent_scope, errors);
        rhs.push_back(expr->get_type(parent_scope));
    }

    for (auto i = 0u; i < vars.size(); ++i) {
        auto& var = vars[i];
        if (i < rhs.size()) {
            var->check_expect(parent_scope, rhs[i], errors);
        } else {
            var->check(parent_scope, errors);
        }
        lhs.push_back(var->get_type(parent_scope));
    }

    const auto lhstype = Type::make_reduced_tuple(std::move(lhs));
    const auto rhstype = Type::make_reduced_tuple(std::move(rhs));
    const auto r = is_assignable(lhstype, rhstype);

    if (!r.yes) {
        errors.emplace_back(to_string(r), location);
    } else if (!r.messages.empty()) {
        errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
    }
}

void NAssignment::dump(std::ostream& out) const {
    bool first = true;
    for (const auto& var : vars) {
        if (!first) {
            out << ",";
        }
        out << *var;
        first = false;
    }

    out << "=";

    first = true;
    for (const auto& expr : exprs) {
        if (!first) {
            out << ",";
        }
        out << *expr;
        first = false;
    }
}

void NEmpty::dump(std::ostream& out) const {
    out << ";";
}

NLabel::NLabel(std::string n)
    : name(std::move(n)) {}

void NLabel::dump(std::ostream& out) const {
    out << "::" << name << "::";
}

void NBreak::dump(std::ostream& out) const {
    out << "break";
}

NGoto::NGoto(std::string n)
    : name(std::move(n)) {}

void NGoto::dump(std::ostream& out) const {
    out << "goto " << name;
}

NWhile::NWhile(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b)
    : condition(std::move(c)), block(std::move(b)) {}

void NWhile::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    condition->check(parent_scope, errors);
    block->check(parent_scope, errors);
}

void NWhile::dump(std::ostream& out) const {
    out << "while " << *condition << " do\n";
    out << *block;
    out << "end";
}

NRepeat::NRepeat(std::unique_ptr<NBlock> b, std::unique_ptr<NExpr> u)
    : block(std::move(b)), until(std::move(u)) {}

void NRepeat::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    block->check(parent_scope, errors);
    until->check(parent_scope, errors);
}

void NRepeat::dump(std::ostream& out) const {
    out << "repeat\n";
    out << *block;
    out << "until " << *until;
}

NElseIf::NElseIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b)
    : condition(std::move(c)), block(std::move(b)) {}

void NElseIf::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    condition->check(parent_scope, errors);
    block->check(parent_scope, errors);
}

void NElseIf::dump(std::ostream& out) const {
    out << "elseif " << *condition << " then\n";
    out << *block;
}

NElse::NElse(std::unique_ptr<NBlock> b)
    : block(std::move(b)) {}

void NElse::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    block->check(parent_scope, errors);
}

void NElse::dump(std::ostream& out) const {
    out << "else\n";
    out << *block;
}

NIf::NIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b, std::vector<std::unique_ptr<NElseIf>> ei, std::unique_ptr<NElse> e)
    : condition(std::move(c)), block(std::move(b)), elseifs(std::move(ei)), else_(std::move(e)) {}

void NIf::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    condition->check(parent_scope, errors);
    block->check(parent_scope, errors);
    for (const auto& elseif : elseifs) {
        elseif->check(parent_scope, errors);
    }
    if (else_) else_->check(parent_scope, errors);
}

void NIf::dump(std::ostream& out) const {
    out << "if " << *condition << " then\n";
    out << *block;
    for (const auto& elseif : elseifs) {
        out << *elseif;
    }
    if (else_) out << *else_;
    out << "end";
}

NForNumeric::NForNumeric(
    std::string n,
    std::unique_ptr<NExpr> b,
    std::unique_ptr<NExpr> e,
    std::unique_ptr<NExpr> s,
    std::unique_ptr<NBlock> k)
    : name(std::move(n)), begin(std::move(b)), end(std::move(e)), step(std::move(s)), block(std::move(k)) {}

void NForNumeric::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    begin->check(parent_scope, errors);
    end->check(parent_scope, errors);
    if (step) step->check(parent_scope, errors);

    auto this_scope = Scope(&parent_scope);
    this_scope.add_name(name, Type(LuaType::NUMBER));

    block->check(this_scope, errors);
}

void NForNumeric::dump(std::ostream& out) const {
    out << "for " << name << "=" << *begin << "," << *end;
    if (step) out << "," << *step;
    out << " do\n";
    out << *block;
    out << "end";
}

NForGeneric::NForGeneric(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e, std::unique_ptr<NBlock> b)
    : names(std::move(n)), exprs(std::move(e)), block(std::move(b)) {}

void NForGeneric::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& name : names) {
        if (parent_scope.get_type_of(name.name)) {
            errors.emplace_back(CompileError::Severity::WARNING, "For-loop variable shadows name `" + name.name + "`", location);
        }
        name.check(parent_scope, errors);
    }

    for (const auto& expr : exprs) {
        expr->check(parent_scope, errors);
    }

    auto this_scope = Scope(&parent_scope);
    for (const auto& name : names) {
        this_scope.add_name(name.name, name.get_type(parent_scope));
    }

    block->check(this_scope, errors);
}

void NForGeneric::dump(std::ostream& out) const {
    out << "for ";
    bool first = true;
    for (const auto& name : names) {
        if (!first) {
            out << ",";
        }
        out << name;
        first = false;
    }
    out << " in ";
    first = true;
    for (const auto& expr : exprs) {
        if (!first) {
            out << ",";
        }
        out << *expr;
        first = false;
    }
    out << " do\n";
    out << *block;
    out << "end";
}

NFuncParams::NFuncParams(std::vector<NNameDecl> n, bool v)
    : names(std::move(n)), is_variadic(v) {}

void NFuncParams::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& name : names) {
        if (parent_scope.get_type_of(name.name)) {
            errors.emplace_back(CompileError::Severity::WARNING, "Function parameter shadows name `" + name.name + "`", location);
        }
        name.check(parent_scope, errors);
    }
}

void NFuncParams::dump(std::ostream& out) const {
    bool first = true;
    for (const auto& name : names) {
        if (!first) {
            out << ",";
        }
        out << name;
        first = false;
    }
    if (is_variadic) {
        if (!first) {
            out << ",";
        }
        out << "...";
    }
}

void NFuncParams::add_to_scope(Scope& scope) const {
    for (const auto& name : names) {
        scope.add_name(name.name, name.get_type(scope));
    }
    if (is_variadic) {
        scope.set_dots_type(Type::make_any());
    } else {
        scope.disable_dots();
    }
}

std::vector<Type> NFuncParams::get_types(const Scope& scope) const {
    std::vector<Type> rv;
    for (const auto& name : names) {
        rv.push_back(name.get_type(scope));
    }
    return rv;
}

FunctionBase::FunctionBase(std::vector<NNameDecl> g, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b)
    : generic_params(std::move(g)), params(std::move(p)), ret(std::move(r)), block(std::move(b)) {}

Type FunctionBase::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto return_type = Type{};

    nominals.clear();
    nominals.reserve(generic_params.size());

    auto setup_scope = [&] {
        auto scope = Scope(&parent_scope);
        auto& deferred = scope.get_deferred_types();

        for (auto i = 0u; i < generic_params.size(); ++i) {
            const auto& gparam = generic_params[i];
            auto defer_id = deferred.reserve(gparam.name);
            deferred.set(defer_id, gparam.get_type(scope));
            scope.add_type(gparam.name, Type::make_nominal(deferred, defer_id));
            nominals.push_back(defer_id);
        }

        params->check(scope, errors);
        params->add_to_scope(scope);

        if (ret) {
            ret->check(scope, errors);
            return_type = ret->get_type(scope);
        }

        if (params->is_variadic) {
            scope.set_dots_type(Type::make_tuple({}, true));
        } else {
            scope.disable_dots();
        }

        return scope;
    };

    if (ret) {
        auto this_scope = setup_scope();

        this_scope.set_return_type(return_type);

        block->check(this_scope, errors);
    } else {
        auto this_scope = setup_scope();

        this_scope.deduce_return_type();

        block->check(this_scope, errors);

        if (auto newret = this_scope.get_return_type()) {
            return_type = std::move(*newret);
        }
    }

    return return_type;
}

Type FunctionBase::get_type(Scope& parent_scope, const Type& rettype) const {
    auto scope = Scope(&parent_scope);
    auto& deferred = scope.get_deferred_types();

    std::vector<NameType> genparams;

    for (auto i = 0u; i < generic_params.size(); ++i) {
        const auto& gparam = generic_params[i];
        scope.add_type(gparam.name, Type::make_nominal(deferred, nominals[i]));
        genparams.push_back({gparam.name, gparam.get_type(scope)});
    }

    return Type::make_function(std::move(genparams), nominals, params->get_types(scope), rettype, params->is_variadic);
}

Type FunctionBase::get_type(Scope& parent_scope, const Type& rettype, const Type& selftype) const {
    auto scope = Scope(&parent_scope);
    auto& deferred = scope.get_deferred_types();

    std::vector<NameType> genparams;

    for (auto i = 0u; i < generic_params.size(); ++i) {
        const auto& gparam = generic_params[i];
        scope.add_type(gparam.name, Type::make_nominal(deferred, nominals[i]));
        genparams.push_back({gparam.name, gparam.get_type(scope)});
    }

    auto paramtypes = params->get_types(scope);

    paramtypes.insert(paramtypes.begin(), selftype);

    return Type::make_function(std::move(genparams), nominals, std::move(paramtypes), rettype, params->is_variadic);
}

NFunction::NFunction(FunctionBase fb, std::unique_ptr<NExpr> e)
    : base(std::move(fb)), expr(std::move(e)) {}

void NFunction::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto return_type = base.check(parent_scope, errors);

    const auto functype = base.get_type(parent_scope, return_type);

    expr->check_expect(parent_scope, functype, errors);
}

void NFunction::dump(std::ostream& out) const {
    out << "function " << *expr << "(" << *base.params << ")";
    out << "\n";
    out << *base.block;
    out << "end";
}

NSelfFunction::NSelfFunction(FunctionBase fb, std::string m, std::unique_ptr<NExpr> e)
    : base(std::move(fb)), name(std::move(m)), expr(std::move(e)) {}

void NSelfFunction::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    expr->check(parent_scope, errors);

    const auto self_type = expr->get_type(parent_scope);

    auto scope = Scope(&parent_scope);

    scope.add_name("self", self_type);

    auto return_type = base.check(scope, errors);

    const auto functype = base.get_type(scope, return_type, self_type);

    if (self_type.get_tag() == Type::Tag::DEFERRED) {
        const auto& defer = self_type.get_deferred();

        if (defer.collection->is_narrowing(defer.id)) {
            const auto& current_type = defer.collection->get(defer.id);

            if (current_type.get_tag() == Type::Tag::TABLE) {
                auto narrowed_type = narrow_field(current_type, name, functype);

                defer.collection->set(defer.id, std::move(narrowed_type));
            }
        }
    }

    std::vector<std::string> notes;

    auto fieldtype = get_field_type(self_type, name, notes, parent_scope.get_luatype_metatable_map());

    if (fieldtype) {
        const auto r = is_assignable(*fieldtype, functype);

        if (!r.yes) {
            errors.emplace_back(to_string(r), location);
        } else if (!r.messages.empty()) {
            errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
        }
    } else {
        if (!notes.empty()) {
            std::string msg;

            for (const auto& note : notes) {
                msg = note + "\n" + msg;
            }

            msg = "Failed to deduce field type\n" + msg;
            errors.emplace_back(msg, location);
        }
    }
}

void NSelfFunction::dump(std::ostream& out) const {
    out << "function " << *expr << ":" << name << "(" << *base.params << ")";
    out << "\n";
    out << *base.block;
    out << "end";
}

NLocalFunction::NLocalFunction(FunctionBase fb, std::string n)
    : base(std::move(fb)), name(std::move(n)) {}

void NLocalFunction::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto return_type = base.check(parent_scope, errors);

    const auto functype = base.get_type(parent_scope, return_type);

    if (auto existing_type = parent_scope.get_type_of(name)) {
        auto r = is_assignable(*existing_type, functype);

        if (!r.yes) {
            errors.emplace_back(to_string(r), location);
        } else if (!r.messages.empty()) {
            errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
        }
    } else {
        parent_scope.add_name(name, functype);
    }
}

void NLocalFunction::dump(std::ostream& out) const {
    out << "local function " << name << "(" << *base.params << ")";
    out << "\n";
    out << *base.block;
    out << "end";
}

NReturn::NReturn(std::vector<std::unique_ptr<NExpr>> e)
    : exprs(std::move(e)) {}

void NReturn::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    auto exprtypes = std::vector<Type>{};

    exprtypes.reserve(exprs.size());

    for (const auto& expr : exprs) {
        expr->check(parent_scope, errors);
        exprtypes.push_back(expr->get_type(parent_scope));
    }

    auto type = Type::make_reduced_tuple(std::move(exprtypes));

    if (auto rettype = parent_scope.get_fixed_return_type()) {
        auto r = is_assignable(*rettype, type);
        if (!r.yes) {
            errors.emplace_back(to_string(r), location);
        }
    } else {
        parent_scope.add_return_type(type);
    }
}

void NReturn::dump(std::ostream& out) const {
    out << "return";
    if (!exprs.empty()) {
        out << " ";
        bool first = true;
        for (const auto& expr : exprs) {
            if (!first) {
                out << ",";
            }
            out << *expr;
            first = false;
        }
    }
}

NLocalVar::NLocalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e)
    : names(std::move(n)), exprs(std::move(e)) {}

void NLocalVar::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& name : names) {
        if (parent_scope.get_type_of(name.name)) {
            errors.emplace_back(CompileError::Severity::WARNING, "Local variable shadows name `" + name.name + "`", location);
        }
        name.check(parent_scope, errors);
    }

    std::vector<Type> exprtypes;
    for (const auto& expr : exprs) {
        expr->check(parent_scope, errors);
        exprtypes.push_back(expr->get_type(parent_scope));
    }

    if (!exprtypes.empty() && exprtypes.back().get_tag() == Type::Tag::TUPLE) {
        auto tupletype = std::move(exprtypes.back());
        exprtypes.pop_back();
        const auto& tuple = tupletype.get_tuple();
        exprtypes.insert(exprtypes.end(), tuple.types.begin(), tuple.types.end());
    }

    for (auto i = 0u; i < names.size(); ++i) {
        const auto& name = names[i];
        if (name.type) {
            parent_scope.add_name(name.name, name.get_type(parent_scope));
        } else if (i < exprtypes.size()) {
            auto& exprtype = exprtypes[i];
            if (exprtype.get_tag() == Type::Tag::LITERAL) {
                auto& collection = parent_scope.get_deferred_types();
                auto id = collection.reserve_narrow("@" + std::to_string(location.first_line));
                collection.set(id, std::move(exprtype));
                exprtype = Type::make_deferred(collection, id);
            }
            parent_scope.add_name(name.name, std::move(exprtype));
        } else {
            parent_scope.add_name(name.name, Type::make_any());
        }
    }
}

void NLocalVar::dump(std::ostream& out) const {
    out << "local ";
    bool first = true;
    for (const auto& name : names) {
        if (!first) {
            out << ",";
        }
        out << name;
        first = false;
    }
    if (!exprs.empty()) {
        out << "=";
        bool first = true;
        for (const auto& expr : exprs) {
            if (!first) {
                out << ",";
            }
            out << *expr;
            first = false;
        }
    }
}

NGlobalVar::NGlobalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e)
    : names(std::move(n)), exprs(std::move(e)) {}

void NGlobalVar::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& name : names) {
        name.check(parent_scope, errors);
    }
    for (const auto& expr : exprs) {
        expr->check(parent_scope, errors);
    }
    for (const auto& name : names) {
        if (auto type = parent_scope.get_type_of(name.name)) {
            auto r = is_assignable(*type, name.get_type(parent_scope));
            if (!r.yes) {
                errors.emplace_back("Global variable conflict: " + to_string(r), location);
            }
        } else {
            parent_scope.add_global_name(name.name, name.get_type(parent_scope));
        }
    }
}

void NGlobalVar::dump(std::ostream& out) const {
    if (!exprs.empty()) {
        bool first = true;
        for (const auto& name : names) {
            if (!first) {
                out << ",";
            }
            out << name;
            first = false;
        }
        out << "=";
        first = true;
        for (const auto& expr : exprs) {
            if (!first) {
                out << ",";
            }
            out << *expr;
            first = false;
        }
    }
}

void NNil::dump(std::ostream& out) const {
    out << "nil";
}

Type NNil::get_type(const Scope& scope) const {
    return Type::make_luatype(LuaType::NIL);
}

NBooleanLiteral::NBooleanLiteral(bool v)
    : value(v) {}

void NBooleanLiteral::dump(std::ostream& out) const {
    out << (value ? "true" : "false");
}

Type NBooleanLiteral::get_type(const Scope& scope) const {
    return Type::make_literal(value);
}

NStringLiteral::NStringLiteral(std::string v)
    : value(std::move(v)) {}

void NStringLiteral::dump(std::ostream& out) const {
    out << value;
}

Type NStringLiteral::get_type(const Scope& scope) const {
    return Type::make_literal(normalize_quotes(value));
}

void NDots::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    if (!parent_scope.get_dots_type()) {
        errors.emplace_back("Scope does not contain `...`", location);
    }
}

void NDots::dump(std::ostream& out) const {
    out << "...";
}

Type NDots::get_type(const Scope& scope) const {
    return Type::make_any();
}

NFunctionDef::NFunctionDef(std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b)
    : params(std::move(p)), ret(std::move(r)), block(std::move(b)) {}

void NFunctionDef::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    params->check(parent_scope, errors);
    if (ret) ret->check(parent_scope, errors);

    if (ret) {
        auto rettype = ret->get_type(parent_scope);

        auto this_scope = Scope(&parent_scope);
        params->add_to_scope(this_scope);
        this_scope.set_return_type(std::move(rettype));

        if (params->is_variadic) {
            this_scope.set_dots_type(Type::make_tuple({}, true));
        } else {
            this_scope.disable_dots();
        }

        block->check(this_scope, errors);
    } else {
        auto this_scope = Scope(&parent_scope);
        params->add_to_scope(this_scope);
        this_scope.deduce_return_type();

        if (params->is_variadic) {
            this_scope.set_dots_type(Type::make_tuple({}, true));
        } else {
            this_scope.disable_dots();
        }

        block->check(this_scope, errors);

        if (auto newret = this_scope.get_return_type()) {
            deducedret = std::move(*newret);
        }
    }
}

void NFunctionDef::dump(std::ostream& out) const {
    out << "function(" << *params << ")";
    out << "\n";
    out << *block;
    out << "end";
}

Type NFunctionDef::get_type(const Scope& scope) const {
    std::vector<Type> paramtypes;
    for (const auto& name : params->names) {
        paramtypes.push_back(name.get_type(scope));
    }

    auto rettype = ret ? ret->get_type(scope) : deducedret ? *deducedret : Type::make_any();

    return Type::make_function(std::move(paramtypes), std::move(rettype), params->is_variadic);
}

NFieldExpr::NFieldExpr(std::unique_ptr<NExpr> e)
    : expr(std::move(e)) {}

void NFieldExpr::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    expr->check(parent_scope, errors);
}

void NFieldExpr::dump(std::ostream& out) const {
    out << *expr;
}

void NFieldExpr::add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
    const {
    auto exprtype = expr->get_type(scope);
    for (auto& index : indexes) {
        if (is_assignable(index.key, LuaType::NUMBER).yes) {
            index.val = index.val | std::move(exprtype);
            return;
        }
    }
    indexes.push_back({Type::make_luatype(LuaType::NUMBER), std::move(exprtype)});
}

NFieldNamed::NFieldNamed(std::string k, std::unique_ptr<NExpr> v)
    : key(std::move(k)), value(std::move(v)) {}

void NFieldNamed::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    value->check(parent_scope, errors);
}

void NFieldNamed::dump(std::ostream& out) const {
    out << key << "=" << *value;
}

void NFieldNamed::add_to_table(
    const Scope& scope,
    std::vector<KeyValPair>& indexes,
    FieldMap& fielddecls,
    std::vector<CompileError>& errors) const {
    auto iter = std::find_if(fielddecls.begin(), fielddecls.end(), [&](NameType& fd) { return fd.name == key; });

    if (iter != fielddecls.end()) {
        errors.emplace_back("Duplicate table key '" + key + "'", location);
        iter->type = iter->type | value->get_type(scope);
    } else {
        fielddecls.push_back({key, value->get_type(scope)});
    }
}

NFieldKey::NFieldKey(std::unique_ptr<NExpr> k, std::unique_ptr<NExpr> v)
    : key(std::move(k)), value(std::move(v)) {}

void NFieldKey::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    key->check(parent_scope, errors);
    value->check(parent_scope, errors);
}

void NFieldKey::dump(std::ostream& out) const {
    out << "[" << *key << "]=" << *value;
}

void NFieldKey::add_to_table(const Scope& scope, std::vector<KeyValPair>& indexes, FieldMap& fielddecls, std::vector<CompileError>& errors)
    const {
    auto keytype = key->get_type(scope);
    auto exprtype = value->get_type(scope);
    for (auto& index : indexes) {
        if (is_assignable(index.key, keytype).yes) {
            index.val = index.val | std::move(exprtype);
            return;
        }
    }
    indexes.push_back({std::move(keytype), std::move(exprtype)});
}

NTableConstructor::NTableConstructor(std::vector<std::unique_ptr<NField>> f)
    : fields(std::move(f)) {}

void NTableConstructor::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    for (const auto& field : fields) {
        field->check(parent_scope, errors);
    }

    std::vector<KeyValPair> indexes;
    FieldMap fielddecls;

    for (const auto& field : fields) {
        field->add_to_table(parent_scope, indexes, fielddecls, errors);
    }

    if (indexes.empty() && fields.empty()) {
        auto& deferred = parent_scope.get_deferred_types();
        auto deferred_id = deferred.reserve_narrow("@" + std::to_string(location.last_line));
        deferred.set(deferred_id, Type::make_table({}, {}));
        cached_type = Type::make_deferred(deferred, deferred_id);
    } else {
        cached_type = Type::make_table(std::move(indexes), std::move(fielddecls));
    }
}

void NTableConstructor::dump(std::ostream& out) const {
    out << "{\n";
    for (const auto& field : fields) {
        out << *field << ",\n";
    }
    out << "}";
}

Type NTableConstructor::get_type(const Scope& scope) const {
    if (cached_type) {
        return *cached_type;
    } else {
        return Type::make_any();
    }
}

NBinop::NBinop(Op o, std::unique_ptr<NExpr> l, std::unique_ptr<NExpr> r)
    : op(o), left(std::move(l)), right(std::move(r)) {}

void NBinop::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    left->check(parent_scope, errors);
    right->check(parent_scope, errors);

    auto lhs = left->get_type(parent_scope);
    auto rhs = right->get_type(parent_scope);

    auto require_compare = [&] {
        for (const auto& type : {LuaType::NUMBER, LuaType::STRING}) {
            auto l = is_assignable(type, lhs);
            auto r = is_assignable(type, rhs);

            if (l.yes && r.yes) {
                return;
            }
        }

        errors.emplace_back("Cannot compare `" + to_string(lhs) + "` to `" + to_string(rhs) + "`", location);
    };

    auto require_equal = [&] {
        auto l = is_assignable(lhs, rhs);
        auto r = is_assignable(rhs, lhs);

        if (l.yes || r.yes) {
            return;
        }

        errors.emplace_back("Cannot compare `" + to_string(lhs) + "` to `" + to_string(rhs) + "`", location);
    };

    auto require_bitwise = [&] {
        auto l = is_assignable(LuaType::NUMBER, lhs);
        auto r = is_assignable(LuaType::NUMBER, rhs);

        if (!l.yes) {
            l.messages.push_back("In bitwise operation");
            errors.emplace_back(to_string(l), location);
        }

        if (!r.yes) {
            r.messages.push_back("In bitwise operation");
            errors.emplace_back(to_string(r), location);
        }
    };

    auto require_concat = [&] {
        auto l = is_assignable(LuaType::STRING, lhs);
        auto r = is_assignable(LuaType::STRING, rhs);

        if (!l.yes) {
            l.messages.push_back("In concat operation");
            errors.emplace_back(to_string(l), location);
        }

        if (!r.yes) {
            r.messages.push_back("In concat operation");
            errors.emplace_back(to_string(r), location);
        }
    };

    auto require_math = [&] {
        auto l = is_assignable(LuaType::NUMBER, lhs);
        auto r = is_assignable(LuaType::NUMBER, rhs);

        if (!l.yes) {
            l.messages.push_back("In arithmetic operation");
            errors.emplace_back(to_string(l), location);
        }

        if (!r.yes) {
            r.messages.push_back("In arithmetic operation");
            errors.emplace_back(to_string(r), location);
        }
    };

    switch (op) {
        case Op::OR: break;
        case Op::AND: break;
        case Op::LT: require_compare(); break;
        case Op::GT: require_compare(); break;
        case Op::LEQ: require_compare(); break;
        case Op::GEQ: require_compare(); break;
        case Op::NEQ: require_equal(); break;
        case Op::EQ: require_equal(); break;
        case Op::BOR: require_bitwise(); break;
        case Op::BXOR: require_bitwise(); break;
        case Op::BAND: require_bitwise(); break;
        case Op::SHL: require_bitwise(); break;
        case Op::SHR: require_bitwise(); break;
        case Op::CONCAT: require_concat(); break;
        case Op::ADD: require_math(); break;
        case Op::SUB: require_math(); break;
        case Op::MUL: require_math(); break;
        case Op::DIV: require_math(); break;
        case Op::IDIV: require_math(); break;
        case Op::MOD: require_math(); break;
        case Op::POW: require_math(); break;
        default: throw std::logic_error("Invalid binary operator");
    }
}

void NBinop::dump(std::ostream& out) const {
    out << "(" << *left << " ";
    switch (op) {
        case Op::OR: out << "or"; break;
        case Op::AND: out << "and"; break;
        case Op::LT: out << "<"; break;
        case Op::GT: out << ">"; break;
        case Op::LEQ: out << "<="; break;
        case Op::GEQ: out << ">="; break;
        case Op::NEQ: out << "~="; break;
        case Op::EQ: out << "=="; break;
        case Op::BOR: out << "|"; break;
        case Op::BXOR: out << "~"; break;
        case Op::BAND: out << "&"; break;
        case Op::SHL: out << "<<"; break;
        case Op::SHR: out << ">>"; break;
        case Op::CONCAT: out << ".."; break;
        case Op::ADD: out << "+"; break;
        case Op::SUB: out << "-"; break;
        case Op::MUL: out << "*"; break;
        case Op::DIV: out << "/"; break;
        case Op::IDIV: out << "//"; break;
        case Op::MOD: out << "%"; break;
        case Op::POW: out << "^"; break;
        default: throw std::logic_error("Invalid binary operator");
    }
    out << " " << *right << ")";
}

Type NBinop::get_type(const Scope& scope) const {
    switch (op) {
        case Op::OR: return (left->get_type(scope) - Type::make_literal(false)) | right->get_type(scope);
        case Op::AND: return Type::make_literal(false) | right->get_type(scope);
        case Op::LT: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::GT: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::LEQ: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::GEQ: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::NEQ: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::EQ: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::BOR: return Type::make_luatype(LuaType::NUMBER);
        case Op::BXOR: return Type::make_luatype(LuaType::NUMBER);
        case Op::BAND: return Type::make_luatype(LuaType::NUMBER);
        case Op::SHL: return Type::make_luatype(LuaType::NUMBER);
        case Op::SHR: return Type::make_luatype(LuaType::NUMBER);
        case Op::CONCAT: return Type::make_luatype(LuaType::STRING);
        case Op::ADD: return Type::make_luatype(LuaType::NUMBER);
        case Op::SUB: return Type::make_luatype(LuaType::NUMBER);
        case Op::MUL: return Type::make_luatype(LuaType::NUMBER);
        case Op::DIV: return Type::make_luatype(LuaType::NUMBER);
        case Op::IDIV: return Type::make_luatype(LuaType::NUMBER);
        case Op::MOD: return Type::make_luatype(LuaType::NUMBER);
        case Op::POW: return Type::make_luatype(LuaType::NUMBER);
        default: throw std::logic_error("Invalid binary operator");
    }
}

NUnaryop::NUnaryop(Op o, std::unique_ptr<NExpr> e)
    : op(o), expr(std::move(e)) {}

void NUnaryop::check(Scope& parent_scope, std::vector<CompileError>& errors) const {
    expr->check(parent_scope, errors);

    auto type = expr->get_type(parent_scope);

    auto require_len = [&] {
        auto str = Type::make_luatype(LuaType::STRING);
        auto tab = Type::make_table({{Type::make_luatype(LuaType::NUMBER), Type::make_any()}}, {});
        auto r = is_assignable(str | tab, type);

        if (!r.yes) {
            r.messages.push_back("In length operator");
            errors.emplace_back(to_string(r), location);
        }
    };

    auto require_number = [&] {
        auto num = Type::make_luatype(LuaType::NUMBER);
        auto r = is_assignable(num, type);

        if (!r.yes) {
            r.messages.push_back("In unary operator");
            errors.emplace_back(to_string(r), location);
        }
    };

    switch (op) {
        case Op::NOT: break;
        case Op::LEN: require_len(); break;
        case Op::NEG: require_number(); break;
        case Op::BNOT: require_number(); break;
        default: throw std::logic_error("Invalid unary operator");
    }
}

void NUnaryop::dump(std::ostream& out) const {
    out << "(";
    switch (op) {
        case Op::NOT: out << "not"; break;
        case Op::LEN: out << "#"; break;
        case Op::NEG: out << "-"; break;
        case Op::BNOT: out << "~"; break;
        default: throw std::logic_error("Invalid unary operator");
    }
    out << " " << *expr << ")";
}

Type NUnaryop::get_type(const Scope& scope) const {
    switch (op) {
        case Op::NOT: return Type::make_luatype(LuaType::BOOLEAN);
        case Op::LEN: return Type::make_luatype(LuaType::NUMBER);
        case Op::NEG: return Type::make_luatype(LuaType::NUMBER);
        case Op::BNOT: return Type::make_luatype(LuaType::NUMBER);
        default: throw std::logic_error("Invalid unary operator");
    }
}

} // namespace typedlua::ast
