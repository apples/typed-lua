#ifndef TYPEDLUA_NODE_HPP
#define TYPEDLUA_NODE_HPP

#include "type.hpp"
#include "scope.hpp"
#include "compile_error.hpp"
#include "location.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace typedlua::ast {

class Node {
public:
    virtual ~Node() = default;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        // do nothing
    }

    virtual void dump(std::ostream& out) const {
        out << "--[[NOT IMPLEMENTED]]";
    }

    Location location;
};

inline std::ostream& operator<<(std::ostream& out, const Node& n) {
    n.dump(out);
    return out;
}

class NExpr : public Node {
public:
    virtual Type get_type(const Scope& scope) const = 0;
};

class NBlock : public Node {
public:
    std::vector<std::unique_ptr<Node>> children;
    bool scoped = false;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        auto this_scope = Scope(&parent_scope);
        for (const auto& child : children) {
            child->check(this_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
        if (scoped) out << "do\n";
        for (const auto& child : children) {
            out << *child << "\n";
        }
        if (scoped) out << "end";
    }
};

class NType : public Node {
public:
    virtual Type get_type(const Scope& scope) const = 0;
};

class NTypeName : public NType {
public:
    NTypeName() = default;
    NTypeName(std::string name) : name(std::move(name)) {}
    std::string name;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (!parent_scope.get_type(name)) {
            errors.emplace_back("Type `" + name + "` not in scope", location);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << name;
    }

    virtual Type get_type(const Scope& scope) const override {
        if (auto type = scope.get_type(name)) {
            return *type;
        } else {
            return Type::make_any();
        }
    }
};

class NTypeFunctionParam : public Node {
public:
    NTypeFunctionParam() = default;
    NTypeFunctionParam(std::string name, std::unique_ptr<NType> type) : name(std::move(name)), type(std::move(type)) {}
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        type->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << name << ":" << *type;
    }
};

class NTypeFunction : public NType {
public:
    NTypeFunction() = default;
    NTypeFunction(std::vector<NTypeFunctionParam> p, std::unique_ptr<NType> r) :
        params(std::move(p)),
        ret(std::move(r)) {}
    std::vector<NTypeFunctionParam> params;
    std::unique_ptr<NType> ret;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& param : params) {
            param.check(parent_scope, errors);
        }
        ret->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "(";
        bool first = true;
        for (const auto& param : params) {
            if (!first) {
                out << ",";
            }
            out << param;
            first = false;
        }
        out << "):" << *ret;
    }

    virtual Type get_type(const Scope& scope) const override {
        std::vector<Type> paramtypes;
        for (const auto& param : params) {
            paramtypes.push_back(param.type->get_type(scope));
        }
        return Type::make_function(std::move(paramtypes), ret->get_type(scope));
    }
};

class NTypeTuple : public NType {
public:
    NTypeTuple() = default;
    NTypeTuple(std::vector<NTypeFunctionParam> p, bool v) : params(std::move(p)), variadic(v) {}
    std::vector<NTypeFunctionParam> params;
    bool variadic;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& param : params) {
            param.check(parent_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "[";
        bool first = true;
        for (const auto& param : params) {
            if (!first) {
                out << ",";
            }
            out << param;
            first = false;
        }
        if (variadic) {
            if (!first) {
                out << ",";
            }
            out << "...";
        }
        out << "]";
    }

    virtual Type get_type(const Scope& scope) const override {
        std::vector<Type> paramtypes;
        for (const auto& param : params) {
            paramtypes.push_back(param.type->get_type(scope));
        }
        return Type::make_tuple(std::move(paramtypes), variadic);
    }
};

class NTypeSum : public NType {
public:
    NTypeSum() = default;
    NTypeSum(std::unique_ptr<NType> l, std::unique_ptr<NType> r): lhs(std::move(l)), rhs(std::move(r)) {}
    std::unique_ptr<NType> lhs;
    std::unique_ptr<NType> rhs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        lhs->check(parent_scope, errors);
        rhs->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "(" << *lhs << "|" << *rhs << ")";
    }

    virtual Type get_type(const Scope& scope) const override {
        return lhs->get_type(scope) | rhs->get_type(scope);
    }
};

class NIndex : public Node {
public:
    NIndex() = default;
    NIndex(std::unique_ptr<NType> k, std::unique_ptr<NType> v) : key(std::move(k)), val(std::move(v)) {}
    std::unique_ptr<NType> key;
    std::unique_ptr<NType> val;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        key->check(parent_scope, errors);
        val->check(parent_scope, errors);

        auto ktype = key->get_type(parent_scope);

        if (is_assignable(ktype, LuaType::NIL).yes) {
            errors.emplace_back("Key type must not be compatible with `nil`", key->location);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "[" << *key << "]:" << *val;
    }

    KeyValPair get_kvp(const Scope& scope) const {
        return {key->get_type(scope), val->get_type(scope)};
    }
};

class NIndexList : public Node {
public:
    NIndexList() = default;
    std::vector<std::unique_ptr<NIndex>> indexes;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& index : indexes) {
            index->check(parent_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
        bool first = true;
        for (const auto& index : indexes) {
            if (!first) {
                out << ";";
            }
            out << *index;
            first = false;
        }
    }

    std::vector<KeyValPair> get_types(const Scope& scope) const {
        std::vector<KeyValPair> rv;
        for (const auto& index : indexes) {
            rv.push_back(index->get_kvp(scope));
        }
        return rv;
    }
};

class NFieldDecl : public Node {
public:
    NFieldDecl() = default;
    NFieldDecl(std::string n, std::unique_ptr<NType> t) : name(std::move(n)), type(std::move(t)) {}
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        type->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << name << ":" << *type;
    }
};

class NFieldDeclList : public Node {
public:
    NFieldDeclList() = default;
    std::vector<std::unique_ptr<NFieldDecl>> fields;
    mutable FieldMap cached_fields;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& field : fields) {
            field->check(parent_scope, errors);
            auto iter = std::find_if(cached_fields.begin(), cached_fields.end(), [&](FieldDecl& fd) {
                return fd.name == field->name;
            });
            if (iter != cached_fields.end()) {
                errors.emplace_back("Duplicate table key '" + field->name + "'", location);
                iter->type = iter->type | field->type->get_type(parent_scope);
            } else {
                cached_fields.push_back({field->name, field->type->get_type(parent_scope)});
            }
        }
    }

    virtual void dump(std::ostream& out) const override {
        bool first = true;
        for (const auto& field : fields) {
            if (!first) {
                out << ";";
            }
            out << *field;
            first = false;
        }
    }

    FieldMap get_types(const Scope& scope) const {
        return cached_fields;
    }
};

class NTypeTable : public NType {
public:
    NTypeTable() = default;
    NTypeTable(std::unique_ptr<NIndexList> i, std::unique_ptr<NFieldDeclList> f) : indexlist(std::move(i)), fieldlist(std::move(f)) {}
    std::unique_ptr<NIndexList> indexlist;
    std::unique_ptr<NFieldDeclList> fieldlist;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (indexlist) indexlist->check(parent_scope, errors);
        if (fieldlist) fieldlist->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "{";
        if (indexlist) out << *indexlist;
        if (fieldlist) out << *fieldlist;
        out << "}";
    }

    virtual Type get_type(const Scope& scope) const override {
        std::vector<KeyValPair> indexes;
        FieldMap fields;

        if (indexlist) indexes = indexlist->get_types(scope);
        if (fieldlist) fields = fieldlist->get_types(scope);

        return Type::make_table(std::move(indexes), std::move(fields));
    }
};

class NInterface : public Node {
public:
    NInterface() = default;
    NInterface(std::string n, std::unique_ptr<NType> t) : name(std::move(n)), type(std::move(t)) {}
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (auto oldtype = parent_scope.get_type(name)) {
            errors.emplace_back(CompileError::Severity::WARNING, "Interface `" + name + "` shadows existing type", location);
        }

        auto& deferred = parent_scope.get_deferred_types();
        const auto deferred_id = deferred.reserve();

        parent_scope.add_type(name, Type::make_deferred(deferred, deferred_id));
        type->check(parent_scope, errors);

        deferred.set(deferred_id, type->get_type(parent_scope));
    }

    virtual void dump(std::ostream& out) const override {
        out << "--[[interface " << name << ":" << *type << "]]";
    }
};

class NNameDecl : public Node {
public:
    NNameDecl() = default;
    NNameDecl(std::string name) : name(std::move(name)) {}
    NNameDecl(std::string name, std::unique_ptr<NType> type) : name(std::move(name)), type(std::move(type)) {}
    std::string name;
    std::unique_ptr<NType> type;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (type) type->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << name;
        if (type) out << "--[[:" << *type << "]]";
    }

    Type get_type(const Scope& scope) const {
        if (type) {
            return type->get_type(scope);
        } else {
            return Type::make_any();
        }
    }
};

class NIdent : public NExpr {
public:
    NIdent() = default;
    NIdent(std::string v) : name(std::move(v)) {}
    std::string name;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (!parent_scope.get_type_of(name)) {
            errors.emplace_back("Name `" + name + "` is not in scope", location);
            // Prevent further type errors for this name
            parent_scope.add_name(name, Type::make_any());
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << name;
    }

    virtual Type get_type(const Scope& scope) const override {
        if (auto type = scope.get_type_of(name)) {
            return *type;
        } else {
            return Type::make_any();
        }
    }
};

class NSubscript : public NExpr {
public:
    NSubscript() = default;
    NSubscript(std::unique_ptr<NExpr> p, std::unique_ptr<NExpr> s) :
        prefix(std::move(p)),
        subscript(std::move(s)) {}
    std::unique_ptr<NExpr> prefix;
    std::unique_ptr<NExpr> subscript;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        prefix->check(parent_scope, errors);
        subscript->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << *prefix << "[" << *subscript << "]";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

class NTableAccess : public NExpr {
public:
    NTableAccess() = default;
    NTableAccess(std::unique_ptr<NExpr> p, std::string n) :
        prefix(std::move(p)),
        name(std::move(n)) {}
    std::unique_ptr<NExpr> prefix;
    std::string name;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        prefix->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << *prefix << "." << name;
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

class NArgSeq : public Node {
public:
    NArgSeq() = default;
    std::vector<std::unique_ptr<NExpr>> args;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& arg : args) {
            arg->check(parent_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
        bool first = true;
        for (const auto& arg : args) {
            if (!first) {
                out << ",";
            }
            out << *arg;
            first = false;
        }
    }
};

class NFunctionCall : public NExpr {
public:
    NFunctionCall() = default;
    NFunctionCall(std::unique_ptr<NExpr> p, std::unique_ptr<NArgSeq> a) :
        prefix(std::move(p)),
        args(std::move(a)) {}
    std::unique_ptr<NExpr> prefix;
    std::unique_ptr<NArgSeq> args;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        prefix->check(parent_scope, errors);
        if (args) args->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << *prefix << "(";
        if (args) out << *args;
        out << ")";
    }

    virtual Type get_type(const Scope& scope) const override {
        auto functype = prefix->get_type(scope);
        switch (functype.get_tag()) {
            case Type::Tag::FUNCTION: return *functype.get_function().ret;
            default: return Type::make_any();
        }
    }
};

class NNumberLiteral : public NExpr {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::ostream& out) const override {
        out << value;
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_luatype(LuaType::NUMBER);
    }
};

class NAssignment : public Node {
public:
    NAssignment() = default;
    NAssignment(std::vector<std::unique_ptr<NExpr>> v, std::vector<std::unique_ptr<NExpr>> e) :
        vars(std::move(v)),
        exprs(std::move(e)) {}
    std::vector<std::unique_ptr<NExpr>> vars;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        std::vector<Type> lhs;
        std::vector<Type> rhs;

        lhs.reserve(vars.size());
        rhs.reserve(exprs.size());

        for (const auto& var : vars) {
            var->check(parent_scope, errors);
            lhs.push_back(var->get_type(parent_scope));
        }

        for (const auto& expr : exprs) {
            expr->check(parent_scope, errors);
            rhs.push_back(expr->get_type(parent_scope));
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

    virtual void dump(std::ostream& out) const override {
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
};

class NEmpty : public Node {
public:
    NEmpty() = default;

    virtual void dump(std::ostream& out) const override {
        out << ";";
    }
};

class NLabel : public Node {
public:
    NLabel() = default;
    NLabel(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::ostream& out) const override {
        out << "::" << name << "::";
    }
};

class NBreak : public Node {
public:
    NBreak() = default;

    virtual void dump(std::ostream& out) const override {
        out << "break";
    }
};

class NGoto : public Node {
public:
    NGoto() = default;
    NGoto(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::ostream& out) const override {
        out << "goto " << name;
    }
};

class NWhile : public Node {
public:
    NWhile() = default;
    NWhile(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b) :
        condition(std::move(c)),
        block(std::move(b)) {}
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        condition->check(parent_scope, errors);
        block->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "while " << *condition << " do\n";
        out << *block;
        out << "end";
    }
};

class NRepeat : public Node {
public:
    NRepeat() = default;
    NRepeat(std::unique_ptr<NBlock> b, std::unique_ptr<NExpr> u) :
        block(std::move(b)),
        until(std::move(u)) {}
    std::unique_ptr<NBlock> block;
    std::unique_ptr<NExpr> until;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        block->check(parent_scope, errors);
        until->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "repeat\n";
        out << *block;
        out << "until " << *until;
    }
};

class NElseIf : public Node {
public:
    NElseIf() = default;
    NElseIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b) :
        condition(std::move(c)),
        block(std::move(b)) {}
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        condition->check(parent_scope, errors);
        block->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "elseif " << *condition << " then\n";
        out << *block;
    }
};

class NElse : public Node {
public:
    NElse() = default;
    NElse(std::unique_ptr<NBlock> b) : block(std::move(b)) {}
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        block->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "else\n";
        out << *block;
    }
};

class NIf : public Node {
public:
    NIf() = default;
    NIf(std::unique_ptr<NExpr> c, std::unique_ptr<NBlock> b, std::vector<std::unique_ptr<NElseIf>> ei, std::unique_ptr<NElse> e) :
        condition(std::move(c)),
        block(std::move(b)),
        elseifs(std::move(ei)),
        else_(std::move(e)) {}
    std::unique_ptr<NExpr> condition;
    std::unique_ptr<NBlock> block;
    std::vector<std::unique_ptr<NElseIf>> elseifs;
    std::unique_ptr<NElse> else_;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        condition->check(parent_scope, errors);
        block->check(parent_scope, errors);
        for (const auto& elseif : elseifs) {
            elseif->check(parent_scope, errors);
        }
        if (else_) else_->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "if " << *condition << " then\n";
        out << *block;
        for (const auto& elseif : elseifs) {
            out << *elseif;
        }
        if (else_) out << *else_;
        out << "end";
    }
};

class NForNumeric : public Node {
public:
    NForNumeric() = default;
    NForNumeric(std::string n, std::unique_ptr<NExpr> b, std::unique_ptr<NExpr> e, std::unique_ptr<NExpr> s, std::unique_ptr<NBlock> k) :
        name(std::move(n)),
        begin(std::move(b)),
        end(std::move(e)),
        step(std::move(s)),
        block(std::move(k)) {}
    std::string name;
    std::unique_ptr<NExpr> begin;
    std::unique_ptr<NExpr> end;
    std::unique_ptr<NExpr> step;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        begin->check(parent_scope, errors);
        end->check(parent_scope, errors);
        if (step) step->check(parent_scope, errors);

        auto this_scope = Scope(&parent_scope);
        this_scope.add_name(name, Type(LuaType::NUMBER));

        block->check(this_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "for " << name << "=" << *begin << "," << *end;
        if (step) out << "," << *step;
        out << " do\n";
        out << *block;
        out << "end";
    }
};

class NForGeneric : public Node {
public:
    NForGeneric() = default;
    NForGeneric(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e, std::unique_ptr<NBlock> b) :
        names(std::move(n)),
        exprs(std::move(e)),
        block(std::move(b)) {}
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto &name : names) {
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

    virtual void dump(std::ostream& out) const override {
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
        for (const auto& expr : exprs ) {
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
};

class NFuncParams : public Node {
public:
    NFuncParams() = default;
    NFuncParams(std::vector<NNameDecl> n, bool v) :
        names(std::move(n)),
        is_variadic(v) {}
    std::vector<NNameDecl> names;
    bool is_variadic = false;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& name : names) {
            if (parent_scope.get_type_of(name.name)) {
                errors.emplace_back(CompileError::Severity::WARNING, "Function parameter shadows name `" + name.name + "`", location);
            }
            name.check(parent_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
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

    void add_to_scope(Scope& scope) const {
        for (const auto& name : names) {
            scope.add_name(name.name, name.get_type(scope));
        }
        if (is_variadic) {
            scope.set_dots_type(Type::make_any());
        } else {
            scope.disable_dots();
        }
    }

    std::vector<Type> get_types(const Scope& scope) const {
        std::vector<Type> rv;
        for (const auto& name : names) {
            rv.push_back(name.get_type(scope));
        }
        return rv;
    }
};

class NFunction : public Node {
public:
    NFunction() = default;
    NFunction(std::unique_ptr<NExpr> e, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b) :
        expr(std::move(e)),
        params(std::move(p)),
        ret(std::move(r)),
        block(std::move(b)) {}
    std::unique_ptr<NExpr> expr;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        expr->check(parent_scope, errors);
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        auto this_scope = Scope(&parent_scope);
        params->add_to_scope(this_scope);

        block->check(this_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "function " << *expr << "(" << *params << ")";
        if (ret) out << "--[[:" << *ret << "]]";
        out << "\n";
        out << *block;
        out << "end";
    }
};

class NSelfFunction : public Node {
public:
    NSelfFunction() = default;
    NSelfFunction(std::string m, std::unique_ptr<NExpr> e, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b) :
        name(std::move(m)),
        expr(std::move(e)),
        params(std::move(p)),
        ret(std::move(r)),
        block(std::move(b)) {}
    std::string name;
    std::unique_ptr<NExpr> expr;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        auto self_type = expr->get_type(parent_scope);
        auto return_type = Type::make_any();

        if (ret) {
            return_type = ret->get_type(parent_scope);

            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.add_name("self", std::move(self_type));
            this_scope.set_return_type(return_type);

            block->check(this_scope, errors);
        } else {
            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.add_name("self", std::move(self_type));
            this_scope.deduce_return_type();

            block->check(this_scope, errors);

            if (auto newret = this_scope.get_return_type()) {
                return_type = std::move(*newret);
            }
        }

        const auto functype = Type::make_function(params->get_types(parent_scope), std::move(return_type));

        const auto r = is_assignable(Type::make_any(), functype);

        if (!r.yes) {
            errors.emplace_back(to_string(r), location);
        } else if (!r.messages.empty()) {
            errors.emplace_back(CompileError::Severity::WARNING, to_string(r), location);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "function " << *expr << ":" << name << "(" << *params << ")";
        if (ret) out << "--[[:" << *ret << "]]";
        out << "\n";
        out << *block;
        out << "end";
    }
};

class NLocalFunction : public Node {
public:
    NLocalFunction() = default;
    NLocalFunction(std::string n, std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b) :
        name(std::move(n)),
        params(std::move(p)),
        ret(std::move(r)),
        block(std::move(b)) {}
    std::string name;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        auto paramtypes = params->get_types(parent_scope);

        if (ret) {
            auto rettype = ret->get_type(parent_scope);
            auto functype = Type::make_function(std::move(paramtypes), rettype);
            parent_scope.add_name(name, std::move(functype));

            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.set_return_type(std::move(rettype));

            block->check(this_scope, errors);
        } else {
            auto functype = Type::make_function(paramtypes, Type::make_any());
            parent_scope.add_name(name, std::move(functype));

            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.deduce_return_type();

            block->check(this_scope, errors);

            if (auto newret = this_scope.get_return_type()) {
                auto newtype = Type::make_function(std::move(paramtypes), std::move(*newret));
                parent_scope.add_name(name, std::move(newtype));
            }
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "local function " << name << "(" << *params << ")";
        if (ret) out << "--[[:" << *ret << "]]";
        out << "\n";
        out << *block;
        out << "end";
    }
};

class NReturn : public Node {
public:
    NReturn() = default;
    NReturn(std::vector<std::unique_ptr<NExpr>> e) : exprs(std::move(e)) {}
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
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

    virtual void dump(std::ostream& out) const override {
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
};

class NLocalVar : public Node {
public:
    NLocalVar() = default;
    NLocalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e) :
        names(std::move(n)),
        exprs(std::move(e)) {}
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
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
                parent_scope.add_name(name.name, std::move(exprtypes[i]));
            } else {
                parent_scope.add_name(name.name, Type::make_any());
            }
        }
    }

    virtual void dump(std::ostream& out) const override {
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
};

class NGlobalVar : public Node {
public:
    NGlobalVar() = default;
    NGlobalVar(std::vector<NNameDecl> n, std::vector<std::unique_ptr<NExpr>> e) :
        names(std::move(n)),
        exprs(std::move(e)) {}
    std::vector<NNameDecl> names;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& name : names) {
            if (parent_scope.get_type_of(name.name)) {
                errors.emplace_back("Global variable shadows name `" + name.name + "`", location);
            }
            name.check(parent_scope, errors);
        }
        for (const auto& expr : exprs) {
            expr->check(parent_scope, errors);
        }
        for (const auto& name : names) {
            parent_scope.add_name(name.name, name.get_type(parent_scope));
        }
    }

    virtual void dump(std::ostream& out) const override {
        if (exprs.empty()) {
            out << "--[[global ";
            bool first = true;
            for (const auto& name : names) {
                if (!first) {
                    out << ",";
                }
                out << name;
                first = false;
            }
            out << "]]";
        } else {
            out << "--[[global]] ";
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
};

class NNil : public NExpr {
public:
    virtual void dump(std::ostream& out) const override {
        out << "nil";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_luatype(LuaType::NIL);
    }
};

class NBooleanLiteral : public NExpr {
public:
    NBooleanLiteral() = default;
    NBooleanLiteral(bool v) : value(v) {}
    bool value;

    virtual void dump(std::ostream& out) const override {
        out << (value ? "true" : "false");
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_luatype(LuaType::BOOLEAN);
    }
};

class NStringLiteral : public NExpr {
public:
    NStringLiteral() = default;
    NStringLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::ostream& out) const override {
        out << value;
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_luatype(LuaType::STRING);
    }
};

class NDots : public NExpr {
public:
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        if (!parent_scope.get_dots_type()) {
            errors.emplace_back("Scope does not contain `...`", location);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "...";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

class NFunctionDef : public NExpr {
public:
    NFunctionDef() = default;
    NFunctionDef(std::unique_ptr<NFuncParams> p, std::unique_ptr<NType> r, std::unique_ptr<NBlock> b) :
        params(std::move(p)),
        ret(std::move(r)),
        block(std::move(b)) {}
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;
    mutable std::optional<Type> deducedret;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        if (ret) {
            auto rettype = ret->get_type(parent_scope);

            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.set_return_type(std::move(rettype));

            block->check(this_scope, errors);
        } else {
            auto this_scope = Scope(&parent_scope);
            params->add_to_scope(this_scope);
            this_scope.deduce_return_type();

            block->check(this_scope, errors);

            if (auto newret = this_scope.get_return_type()) {
                deducedret = std::move(*newret);
            }
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "function(" << *params << ")";
        if (ret) out << "--[[:" << *ret << "]]";
        out << "\n";
        out << *block;
        out << "end";
    }

    virtual Type get_type(const Scope& scope) const override {
        std::vector<Type> paramtypes;
        for (const auto& name : params->names) {
            paramtypes.push_back(name.get_type(scope));
        }

        auto rettype =
            ret ? ret->get_type(scope) :
            deducedret ? *deducedret :
            Type::make_any();

        return Type::make_function(std::move(paramtypes), std::move(rettype));
    }
};

class NField : public Node {
public:
    virtual void add_to_table(
        const Scope& scope,
        std::vector<KeyValPair>& indexes,
        FieldMap& fielddecls,
        std::vector<CompileError>& errors) const = 0;
};

class NFieldExpr : public NField {
public:
    NFieldExpr() = default;
    NFieldExpr(std::unique_ptr<NExpr> e) : expr(std::move(e)) {}
    std::unique_ptr<NExpr> expr;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        expr->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << *expr;
    }

    virtual void add_to_table(
        const Scope& scope,
        std::vector<KeyValPair>& indexes,
        FieldMap& fielddecls,
        std::vector<CompileError>& errors) const override
    {
        auto exprtype = expr->get_type(scope);
        for (auto& index : indexes) {
            if (is_assignable(index.key, LuaType::NUMBER).yes) {
                index.val = index.val | std::move(exprtype);
                return;
            }
        }
        indexes.push_back({Type::make_luatype(LuaType::NUMBER), std::move(exprtype)});
    }
};

class NFieldNamed : public NField {
public:
    NFieldNamed() = default;
    NFieldNamed(std::string k, std::unique_ptr<NExpr> v) :
        key(std::move(k)),
        value(std::move(v)) {}
    std::string key;
    std::unique_ptr<NExpr> value;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        value->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << key << "=" << *value;
    }

    virtual void add_to_table(
        const Scope& scope,
        std::vector<KeyValPair>& indexes,
        FieldMap& fielddecls,
        std::vector<CompileError>& errors) const override
    {
        auto iter = std::find_if(fielddecls.begin(), fielddecls.end(), [&](FieldDecl& fd) {
            return fd.name == key;
        });
        if (iter != fielddecls.end()) {
            errors.emplace_back("Duplicate table key '" + key + "'", location);
            iter->type = iter->type | value->get_type(scope);
        } else {
            fielddecls.push_back({key, value->get_type(scope)});
        }
    }
};

class NFieldKey : public NField {
public:
    NFieldKey() = default;
    NFieldKey(std::unique_ptr<NExpr> k, std::unique_ptr<NExpr> v) :
        key(std::move(k)),
        value(std::move(v)) {}
    std::unique_ptr<NExpr> key;
    std::unique_ptr<NExpr> value;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        key->check(parent_scope, errors);
        value->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "[" << *key << "]=" << *value;
    }

    virtual void add_to_table(
        const Scope& scope,
        std::vector<KeyValPair>& indexes,
        FieldMap& fielddecls,
        std::vector<CompileError>& errors) const override
    {
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
};

class NTableConstructor : public NExpr {
public:
    NTableConstructor() = default;
    NTableConstructor(std::vector<std::unique_ptr<NField>> f) : fields(std::move(f)) {}
    std::vector<std::unique_ptr<NField>> fields;
    mutable std::optional<Type> cached_type;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& field : fields) {
            field->check(parent_scope, errors);
        }

        std::vector<KeyValPair> indexes;
        FieldMap fielddecls;

        for (const auto& field : fields) {
            field->add_to_table(parent_scope, indexes, fielddecls, errors);
        }

        if (indexes.empty() && fielddecls.empty()) {
            indexes.push_back({Type::make_any(), Type::make_any()});
        }

        cached_type = Type::make_table(std::move(indexes), std::move(fielddecls));
    }

    virtual void dump(std::ostream& out) const override {
        out << "{\n";
        for (const auto& field : fields) {
            out << *field << ",\n";
        }
        out << "}";
    }

    virtual Type get_type(const Scope& scope) const override {
        if (cached_type) {
            return *cached_type;
        } else {
            return Type::make_any();
        }
    }
};

class NBinop : public NExpr {
public:
    enum class Op {
        OR,
        AND,
        LT,
        GT,
        LEQ,
        GEQ,
        NEQ,
        EQ,
        BOR,
        BXOR,
        BAND,
        SHL,
        SHR,
        CONCAT,
        ADD,
        SUB,
        MUL,
        DIV,
        IDIV,
        MOD,
        POW
    };

    NBinop() = default;
    NBinop(Op o, std::unique_ptr<NExpr> l, std::unique_ptr<NExpr> r) :
        op(o),
        left(std::move(l)),
        right(std::move(r)) {}
    Op op;
    std::unique_ptr<NExpr> left;
    std::unique_ptr<NExpr> right;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        left->check(parent_scope, errors);
        right->check(parent_scope, errors);

        auto lhs = left->get_type(parent_scope);
        auto rhs = right->get_type(parent_scope);

        auto require_compare = [&]{
            for (const auto& type : {LuaType::NUMBER, LuaType::STRING}) {
                auto l = is_assignable(type, lhs);
                auto r = is_assignable(type, rhs);

                if (l.yes && r.yes) {
                    return;
                }
            }

            errors.emplace_back("Cannot compare `" + to_string(lhs) + "` to `" + to_string(rhs) + "`", location);
        };

        auto require_equal = [&]{
            auto l = is_assignable(lhs, rhs);
            auto r = is_assignable(rhs, lhs);

            if (l.yes || r.yes) {
                return;
            }

            errors.emplace_back("Cannot compare `" + to_string(lhs) + "` to `" + to_string(rhs) + "`", location);
        };

        auto require_bitwise = [&]{
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

        auto require_concat = [&]{
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

        auto require_math = [&]{
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

    virtual void dump(std::ostream& out) const override {
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

    virtual Type get_type(const Scope& scope) const override {
        switch (op) {
            case Op::OR:
                return (left->get_type(scope) - Type::make_literal(false))
                    | right->get_type(scope);
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
};

class NUnaryop : public NExpr {
public:
    enum class Op {
        NOT,
        LEN,
        NEG,
        BNOT
    };

    NUnaryop() = default;
    NUnaryop(Op o, std::unique_ptr<NExpr> e) :
        op(o),
        expr(std::move(e)) {}
    Op op;
    std::unique_ptr<NExpr> expr;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        expr->check(parent_scope, errors);

        auto type = expr->get_type(parent_scope);

        auto require_len = [&]{
            auto str = Type::make_luatype(LuaType::STRING);
            auto tab = Type::make_table({{Type::make_luatype(LuaType::NUMBER), Type::make_any()}}, {});
            auto r = is_assignable(str | tab, type);

            if (!r.yes) {
                r.messages.push_back("In length operator");
                errors.emplace_back(to_string(r), location);
            }
        };

        auto require_number = [&]{
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

    virtual void dump(std::ostream& out) const override {
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

    virtual Type get_type(const Scope& scope) const override {
        switch (op) {
            case Op::NOT: return Type::make_luatype(LuaType::BOOLEAN);
            case Op::LEN: return Type::make_luatype(LuaType::NUMBER);
            case Op::NEG: return Type::make_luatype(LuaType::NUMBER);
            case Op::BNOT: return Type::make_luatype(LuaType::NUMBER);
            default: throw std::logic_error("Invalid unary operator");
        }
    }
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
