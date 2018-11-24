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

class NNumberLiteral : public Node {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v) : value(std::move(v)) {}
    std::string value;

    virtual void dump(std::string prefix) const override {
        std::cout << prefix << "(NNumberLiteral " << value << ")\n";
    }
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
