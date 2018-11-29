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
    virtual void dump(std::string prefix) const {
        std::cout << prefix << "(Node)\n";
    }
};

class NExpr : public Node {

};

class NBlock : public Node {
public:
    std::vector<std::unique_ptr<Node>> children;

    virtual void dump(std::string prefix) const override {
        std::cout << prefix << "(NBlock" << "\n";
        prefix += "  ";
        for (const auto& child : children) {
            child->dump(prefix);
        }
        std::cout << prefix << ")" << "\n";
    }
};

class NIdent : public NExpr {
public:
    NIdent() = default;
    NIdent(std::string v) : name(std::move(v)) {}
    std::string name;

    virtual void dump(std::string prefix) const override {
        std::cout << prefix << "(NIdent " << name << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NSubscript\n";
        indent += "  ";
        prefix->dump(indent);
        subscript->dump(indent);
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NTableAccess\n";
        indent += "  ";
        prefix->dump(indent);
        std::cout << indent << name << "\n";
        std::cout << indent << ")\n";
    }
};

class NArgSeq : public Node {
public:
    NArgSeq() = default;
    std::vector<std::unique_ptr<NExpr>> args;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NArgSeq\n";
        indent += "  ";
        for (const auto& arg : args) {
            arg->dump(indent);
        }
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NFunctionCall\n";
        indent += "  ";
        prefix->dump(indent);
        if (args) {
            args->dump(indent);
        }
        std::cout << indent << ")\n";
    }
};

class NNumberLiteral : public NExpr {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::string prefix) const override {
        std::cout << prefix << "(NNumberLiteral " << value << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NAssignment\n";
        indent += "  ";
        const auto indent2 = indent + "  ";
        std::cout << indent << "([vars]\n";
        for (const auto& var : vars) {
            var->dump(indent2);
        }
        std::cout << indent2 << ")\n";
        std::cout << indent << "([exprs]\n";
        for (const auto& expr : exprs) {
            expr->dump(indent2);
        }
        std::cout << indent2 << ")\n";
        std::cout << indent << ")\n";
    }
};

class NEmpty : public Node {
public:
    NEmpty() = default;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NEmpty)\n";
    }
};

class NLabel : public Node {
public:
    NLabel() = default;
    NLabel(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NLabel " << name << ")\n";
    }
};

class NBreak : public Node {
public:
    NBreak() = default;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NBreak)\n";
    }
};

class NGoto : public Node {
public:
    NGoto() = default;
    NGoto(std::string n) : name(std::move(n)) {}
    std::string name;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NGoto " << name << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NWhile\n";
        indent += "  ";
        condition->dump(indent);
        block->dump(indent);
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NRepeat\n";
        indent += "  ";
        block->dump(indent);
        until->dump(indent);
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NElseIf\n";
        indent += "  ";
        condition->dump(indent);
        block->dump(indent);
        std::cout << indent << ")\n";
    }
};

class NElse : public Node {
public:
    NElse() = default;
    NElse(std::unique_ptr<NBlock> b) : block(std::move(b)) {}
    std::unique_ptr<NBlock> block;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NElse\n";
        indent += "  ";
        block->dump(indent);
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NIf\n";
        indent += "  ";
        condition->dump(indent);
        block->dump(indent);
        for (const auto& elseif : elseifs) {
            elseif->dump(indent);
        }
        if (else_) {
            else_->dump(indent);
        }
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NForNumeric\n";
        indent += "  ";
        std::cout << indent << name << "\n";
        begin->dump(indent);
        end->dump(indent);
        if (step) {
            step->dump(indent);
        }
        block->dump(indent);
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NForGeneric\n";
        indent += "  ";
        auto indent2 = indent + "  ";
        std::cout << indent << "([names]\n";
        for (const auto& name : names) {
            std::cout << indent2 << name << "\n";
        }
        std::cout << indent2 << ")" << "\n";
        std::cout << indent << "([exprs]\n";
        for (const auto& expr : exprs) {
            expr->dump(indent2);
        }
        std::cout << indent2 << ")\n";
        block->dump(indent);
        std::cout << indent << ")\n";
    }
};

class NFunction : public Node {
public:
    NFunction() = default;
    NFunction(std::unique_ptr<NExpr> e, std::vector<std::string> p, std::unique_ptr<NBlock> b) :
        expr(std::move(e)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::unique_ptr<NExpr> expr;
    std::vector<std::string> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NFunction\n";
        indent += "  ";
        expr->dump(indent);
        std::cout << indent << "([params]\n";
        const auto indent2 = indent + "  ";
        for (const auto& param : params) {
            std::cout << indent2 << param << "\n";
        }
        std::cout << indent2 << ")\n";
        block->dump(indent);
        std::cout << indent << ")\n";
    }
};

class NSelfFunction : public Node {
public:
    NSelfFunction() = default;
    NSelfFunction(std::string m, std::unique_ptr<NExpr> e, std::vector<std::string> p, std::unique_ptr<NBlock> b) :
        name(std::move(m)),
        expr(std::move(e)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::string name;
    std::unique_ptr<NExpr> expr;
    std::vector<std::string> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NSelfFunction\n";
        indent += "  ";
        std::cout << indent << name << "\n";
        expr->dump(indent);
        std::cout << indent << "([params]\n";
        const auto indent2 = indent + "  ";
        for (const auto& param : params) {
            std::cout << indent2 << param << "\n";
        }
        std::cout << indent2 << ")\n";
        block->dump(indent);
        std::cout << indent << ")\n";
    }
};

class NLocalFunction : public Node {
public:
    NLocalFunction() = default;
    NLocalFunction(std::string n, std::vector<std::string> p, std::unique_ptr<NBlock> b) :
        name(std::move(n)),
        params(std::move(p)),
        block(std::move(b)) {}
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<NBlock> block;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NLocalFunction\n";
        indent += "  ";
        std::cout << indent << name << "\n";
        std::cout << indent << "([params]\n";
        const auto indent2 = indent + "  ";
        for (const auto& param : params) {
            std::cout << indent2 << param << "\n";
        }
        std::cout << indent2 << ")\n";
        block->dump(indent);
        std::cout << indent << ")\n";
    }
};

class NReturn : public Node {
public:
    NReturn() = default;
    NReturn(std::vector<std::unique_ptr<NExpr>> e) : exprs(std::move(e)) {}
    std::vector<std::unique_ptr<NExpr>> exprs;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NReturn\n";
        indent += "  ";
        for (const auto& expr : exprs) {
            expr->dump(indent);
        }
        std::cout << indent << ")\n";
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

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NLocalVar\n";
        indent += "  ";
        for (const auto& name : names) {
            std::cout << indent << name << "\n";
        }
        for (const auto& expr : exprs) {
            expr->dump(indent);
        }
        std::cout << indent << ")\n";
    }
};

class NNil : public NExpr {
public:
    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NNil)\n";
    }
};

class NBooleanLiteral : public NExpr {
public:
    NBooleanLiteral() = default;
    NBooleanLiteral(bool v) : value(v) {}
    bool value;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NBooleanLiteral "
            << (value ? "true" : "false") << ")\n";
    }
};

class NStringLiteral : public NExpr {
public:
    NStringLiteral() = default;
    NStringLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::string prefix) const override {
        std::cout << prefix << "(NStringLiteral " << value << ")\n";
    }
};

class NDots : public NExpr {
public:
    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NDots)\n";
    }
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
