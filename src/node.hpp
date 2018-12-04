#ifndef TYPEDLUA_NODE_HPP
#define TYPEDLUA_NODE_HPP

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace typedlua::ast {

class Node {
public:
    virtual ~Node() = default;
    virtual void dump(std::ostream& out, std::string prefix) const {
        out << prefix << "(Node)\n";
    }
};

class NExpr : public Node {

};

class NBlock : public Node {
public:
    std::vector<std::unique_ptr<Node>> children;

    virtual void dump(std::ostream& out, std::string prefix) const override {
        out << prefix << "(NBlock" << "\n";
        prefix += "  ";
        for (const auto& child : children) {
            child->dump(out, prefix);
        }
        out << prefix << ")" << "\n";
    }
};

class NIdent : public NExpr {
public:
    NIdent() = default;
    NIdent(std::string v) : name(std::move(v)) {}
    std::string name;

    virtual void dump(std::ostream& out, std::string prefix) const override {
        out << prefix << "(NIdent " << name << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NSubscript\n";
        indent += "  ";
        prefix->dump(out, indent);
        subscript->dump(out, indent);
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NTableAccess\n";
        indent += "  ";
        prefix->dump(out, indent);
        out << indent << name << "\n";
        out << indent << ")\n";
    }
};

class NArgSeq : public Node {
public:
    NArgSeq() = default;
    std::vector<std::unique_ptr<NExpr>> args;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NArgSeq\n";
        indent += "  ";
        for (const auto& arg : args) {
            arg->dump(out, indent);
        }
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFunctionCall\n";
        indent += "  ";
        prefix->dump(out, indent);
        if (args) {
            args->dump(out, indent);
        }
        out << indent << ")\n";
    }
};

class NNumberLiteral : public NExpr {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::ostream& out, std::string prefix) const override {
        out << prefix << "(NNumberLiteral " << value << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NAssignment\n";
        indent += "  ";
        const auto indent2 = indent + "  ";
        out << indent << "([vars]\n";
        for (const auto& var : vars) {
            var->dump(out, indent2);
        }
        out << indent2 << ")\n";
        out << indent << "([exprs]\n";
        for (const auto& expr : exprs) {
            expr->dump(out, indent2);
        }
        out << indent2 << ")\n";
        out << indent << ")\n";
    }
};

class NEmpty : public Node {
public:
    NEmpty() = default;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NEmpty)\n";
    }
};

class NLabel : public Node {
public:
    NLabel() = default;
    NLabel(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NLabel " << name << ")\n";
    }
};

class NBreak : public Node {
public:
    NBreak() = default;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NBreak)\n";
    }
};

class NGoto : public Node {
public:
    NGoto() = default;
    NGoto(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NGoto " << name << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NWhile\n";
        indent += "  ";
        condition->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NRepeat\n";
        indent += "  ";
        block->dump(out, indent);
        until->dump(out, indent);
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NElseIf\n";
        indent += "  ";
        condition->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NElse : public Node {
public:
    NElse() = default;
    NElse(std::unique_ptr<NBlock> b) : block(std::move(b)) {}
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NElse\n";
        indent += "  ";
        block->dump(out, indent);
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NIf\n";
        indent += "  ";
        condition->dump(out, indent);
        block->dump(out, indent);
        for (const auto& elseif : elseifs) {
            elseif->dump(out, indent);
        }
        if (else_) {
            else_->dump(out, indent);
        }
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NForNumeric\n";
        indent += "  ";
        out << indent << name << "\n";
        begin->dump(out, indent);
        end->dump(out, indent);
        if (step) {
            step->dump(out, indent);
        }
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NForGeneric : public Node {
public:
    NForGeneric() = default;
    NForGeneric(std::vector<std::string> n, std::vector<std::unique_ptr<NExpr>> e, std::unique_ptr<NBlock> b) :
        names(std::move(n)),
        exprs(std::move(e)),
        block(std::move(b)) {}
    std::vector<std::string> names;
    std::vector<std::unique_ptr<NExpr>> exprs;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NForGeneric\n";
        indent += "  ";
        auto indent2 = indent + "  ";
        out << indent << "([names]\n";
        for (const auto& name : names) {
            out << indent2 << name << "\n";
        }
        out << indent2 << ")" << "\n";
        out << indent << "([exprs]\n";
        for (const auto& expr : exprs) {
            expr->dump(out, indent2);
        }
        out << indent2 << ")\n";
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NFuncParams : public Node {
public:
    NFuncParams() = default;
    NFuncParams(std::vector<std::string> n, bool v) :
        names(std::move(n)),
        is_variadic(v) {}
    std::vector<std::string> names;
    bool is_variadic = false;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFuncParams\n";
        indent += "  ";
        for (const auto& name : names) {
            out << indent << name << "\n";
        }
        if (is_variadic) {
            out << indent << "...\n";
        }
        out << indent << ")\n";
    }
};

class NFunction : public Node {
public:
    NFunction() = default;
    NFunction(std::unique_ptr<NExpr> e, std::unique_ptr<NFuncParams> p, std::unique_ptr<NBlock> b) :
        expr(std::move(e)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::unique_ptr<NExpr> expr;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFunction\n";
        indent += "  ";
        expr->dump(out, indent);
        params->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NSelfFunction : public Node {
public:
    NSelfFunction() = default;
    NSelfFunction(std::string m, std::unique_ptr<NExpr> e, std::unique_ptr<NFuncParams> p, std::unique_ptr<NBlock> b) :
        name(std::move(m)),
        expr(std::move(e)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::string name;
    std::unique_ptr<NExpr> expr;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NSelfFunction\n";
        indent += "  ";
        out << indent << name << "\n";
        expr->dump(out, indent);
        params->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NLocalFunction : public Node {
public:
    NLocalFunction() = default;
    NLocalFunction(std::string n, std::unique_ptr<NFuncParams> p, std::unique_ptr<NBlock> b) :
        name(std::move(n)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::string name;
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NLocalFunction\n";
        indent += "  ";
        out << indent << name << "\n";
        params->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NReturn : public Node {
public:
    NReturn() = default;
    NReturn(std::vector<std::unique_ptr<NExpr>> e) : exprs(std::move(e)) {}
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NReturn\n";
        indent += "  ";
        for (const auto& expr : exprs) {
            expr->dump(out, indent);
        }
        out << indent << ")\n";
    }
};

class NLocalVar : public Node {
public:
    NLocalVar() = default;
    NLocalVar(std::vector<std::string> n, std::vector<std::unique_ptr<NExpr>> e) :
        names(std::move(n)),
        exprs(std::move(e)) {}
    std::vector<std::string> names;
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NLocalVar\n";
        indent += "  ";
        for (const auto& name : names) {
            out << indent << name << "\n";
        }
        for (const auto& expr : exprs) {
            expr->dump(out, indent);
        }
        out << indent << ")\n";
    }
};

class NNil : public NExpr {
public:
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NNil)\n";
    }
};

class NBooleanLiteral : public NExpr {
public:
    NBooleanLiteral() = default;
    NBooleanLiteral(bool v) : value(v) {}
    bool value;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NBooleanLiteral "
            << (value ? "true" : "false") << ")\n";
    }
};

class NStringLiteral : public NExpr {
public:
    NStringLiteral() = default;
    NStringLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::ostream& out, std::string prefix) const override {
        out << prefix << "(NStringLiteral " << value << ")\n";
    }
};

class NDots : public NExpr {
public:
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NDots)\n";
    }
};

class NFunctionDef : public NExpr {
public:
    NFunctionDef() = default;
    NFunctionDef(std::unique_ptr<NFuncParams> p, std::unique_ptr<NBlock> b) :
        params(std::move(p)),
        block(std::move(b)) {}
    std::unique_ptr<NFuncParams> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFunctionDef\n";
        indent += "  ";
        params->dump(out, indent);
        block->dump(out, indent);
        out << indent << ")\n";
    }
};

class NField : public Node {
};

class NFieldExpr : public NField {
public:
    NFieldExpr() = default;
    NFieldExpr(std::unique_ptr<NExpr> e) : expr(std::move(e)) {}
    std::unique_ptr<NExpr> expr;
    
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFieldExpr\n";
        indent += "  ";
        expr->dump(out, indent);
        out << indent << ")\n";
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
    
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFieldNamed\n";
        indent += "  ";
        out << indent << key << "\n";
        value->dump(out, indent);
        out << indent << ")\n";
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
    
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NFieldKey\n";
        indent += "  ";
        key->dump(out, indent);
        value->dump(out, indent);
        out << indent << ")\n";
    }
};

class NTableConstructor : public NExpr {
public:
    NTableConstructor() = default;
    NTableConstructor(std::vector<std::unique_ptr<NField>> f) : fields(std::move(f)) {}
    std::vector<std::unique_ptr<NField>> fields;
    
    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NTableConstructor\n";
        indent += "  ";
        for (const auto& field : fields) {
            field->dump(out, indent);
        }
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NBinop '" << opname << "'\n";
        indent += "  ";
        left->dump(out, indent);
        right->dump(out, indent);
        out << indent << ")\n";
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

    virtual void dump(std::ostream& out, std::string indent) const override {
        out << indent << "(NUnaryop '" << opname << "'\n";
        indent += "  ";
        expr->dump(out, indent);
        out << indent << ")\n";
    }
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
