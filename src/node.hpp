#ifndef TYPEDLUA_NODE_HPP
#define TYPEDLUA_NODE_HPP

#include <memory>
#include <string>
#include <vector>

namespace typedlua::ast {

class Node {
public:
    virtual ~Node() = default;
};

class NBlock : public Node {
public:
    std::vector<std::unique_ptr<Node>> children;
};

class NNumberLiteral : public Node {
public:
    NNumberLiteral() = default;
    NNumberLiteral(std::string v) : value(std::move(v)) {}
    std::string value;
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
