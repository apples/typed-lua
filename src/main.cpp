#include <iostream>
#include "node.hpp"
extern int yyparse(typedlua::ast::Node*& root);

int main(int argc, char **argv)
{
    typedlua::ast::Node* root = nullptr;
    yyparse(root);
}
