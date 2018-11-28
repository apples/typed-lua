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
    NAssignment(std::unique_ptr<NExpr> d, std::unique_ptr<NExpr> v) :
        dest(std::move(d)),
        value(std::move(v)) {}
    std::unique_ptr<NExpr> dest;
    std::unique_ptr<NExpr> value;

    virtual void dump(std::string indent) const override {
        std::cout << indent << "(NAssignment\n";
        indent += "  ";
        dest->dump(indent);
        value->dump(indent);
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

class NForIter : public Node {
public:
    NForIter() = default;
    NForIter(std::string n, std::unique_ptr<NExpr> b, std::unique_ptr<NExpr> e, std::unique_ptr<NExpr> s, std::unique_ptr<NBlock> k) :
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
        std::cout << indent << "(NForIter\n";
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

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
