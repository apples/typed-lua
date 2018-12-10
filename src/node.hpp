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
        for (const auto& var : vars) {
            var->check(parent_scope, errors);
        }

        for (const auto& expr : exprs) {
            expr->check(parent_scope, errors);
        }

        for (auto i = 0u; i < exprs.size(); ++i) {
            if (i < vars.size()) {
                auto r = is_assignable(vars[i]->get_type(parent_scope), exprs[i]->get_type(parent_scope));
                if (!r.yes) {
                    errors.emplace_back(to_string(r), location);
                }
            } else {
                errors.emplace_back(CompileError::Severity::WARNING, "Too many expressions on right side of assignment", location);
                break;
            }
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
        expr->check(parent_scope, errors);
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        auto this_scope = Scope(&parent_scope);
        params->add_to_scope(this_scope);

        block->check(this_scope, errors);
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

        auto this_scope = Scope(&parent_scope);
        this_scope.add_name(name, Type::make_any());
        params->add_to_scope(this_scope);

        block->check(this_scope, errors);
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
        for (const auto& expr : exprs) {
            expr->check(parent_scope, errors);
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
        for (const auto& expr : exprs) {
            expr->check(parent_scope, errors);
        }
        for (const auto& name : names) {
            parent_scope.add_name(name.name, name.get_type(parent_scope));
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
        block(std::move(b)) {}
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NType> ret;
    std::unique_ptr<NBlock> block;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        params->check(parent_scope, errors);
        if (ret) ret->check(parent_scope, errors);

        auto this_scope = Scope(&parent_scope);
        params->add_to_scope(this_scope);

        block->check(this_scope, errors);
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
        auto rettype = ret ? ret->get_type(scope) : Type::make_any();
        return Type::make_function(std::move(paramtypes), std::move(rettype));
    }
};

class NField : public Node {
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
};

class NTableConstructor : public NExpr {
public:
    NTableConstructor() = default;
    NTableConstructor(std::vector<std::unique_ptr<NField>> f) : fields(std::move(f)) {}
    std::vector<std::unique_ptr<NField>> fields;
    
    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        for (const auto& field : fields) {
            field->check(parent_scope, errors);
        }
    }

    virtual void dump(std::ostream& out) const override {
        out << "{\n";
        for (const auto& field : fields) {
            out << *field << ",\n";
        }
        out << "}";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

class NBinop : public NExpr {
public:
    NBinop() = default;
    NBinop(std::string o, std::unique_ptr<NExpr> l, std::unique_ptr<NExpr> r) :
        opname(std::move(o)),
        left(std::move(l)),
        right(std::move(r)) {}
    std::string opname;
    std::unique_ptr<NExpr> left;
    std::unique_ptr<NExpr> right;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        left->check(parent_scope, errors);
        right->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "(" << *left << " " << opname << " " << *right << ")";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

class NUnaryop : public NExpr {
public:
    NUnaryop() = default;
    NUnaryop(std::string o, std::unique_ptr<NExpr> e) :
        opname(std::move(o)),
        expr(std::move(e)) {}
    std::string opname;
    std::unique_ptr<NExpr> expr;

    virtual void check(Scope& parent_scope, std::vector<CompileError>& errors) const {
        expr->check(parent_scope, errors);
    }

    virtual void dump(std::ostream& out) const override {
        out << "(" << opname << " " << *expr << ")";
    }

    virtual Type get_type(const Scope& scope) const override {
        return Type::make_any();
    }
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
