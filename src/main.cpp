#include <iostream>
#include "node.hpp"
extern int yyparse(typedlua::ast::Node*& root);
extern int yydebug;

int main(int argc, char **argv) {
    typedlua::ast::Node* root = nullptr;
    if (yyparse(root) == 0) {
        if (root) {
            root->dump("");
        }
    }
}
