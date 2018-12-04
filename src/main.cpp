#include <iostream>
#include "node.hpp"
#include "parser.hpp"
#include "lexer.hpp"

int main(int argc, char **argv) {
    yyscan_t scanner;
    typedlua_lex_init(&scanner);
    typedlua::ast::Node* root = nullptr;
    if (typedlua_parse(scanner, root) == 0) {
        if (root) {
            root->dump("");
        }
    }
    typedlua_lex_destroy(scanner);
}
