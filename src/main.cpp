#include <iostream>
#include "node.hpp"
#include "parser.hpp"
#include "lexer.hpp"

int main(int argc, char **argv) {
    yyscan_t scanner;
    yylex_init(&scanner);
    typedlua::ast::Node* root = nullptr;
    if (yyparse(scanner, root) == 0) {
        if (root) {
            root->dump("");
        }
    }
    yylex_destroy(scanner);
}
