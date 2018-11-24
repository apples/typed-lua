#ifndef TYPEDLUA_NODE_HPP
#define TYPEDLUA_NODE_HPP

#include <string>

namespace typedlua::ast {

class node {
public:
    virtual ~node() = default;
};

class number_literal : public node {
public:
    number_literal() = default;
    number_literal(std::string v) : value(std::move(v)) {}
    std::string value;
};

} // namespace typedlua::ast

#endif // TYPEDLUA_NODE_HPP
