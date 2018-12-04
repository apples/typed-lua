#include "typedlua_compiler.hpp"

#include "parser.hpp"
#include "lexer.hpp"

#include <sstream>

typedlua_compiler::typedlua_compiler() {}

auto typedlua_compiler::compile(std::string_view source, std::string_view name) -> std::string {
    yyscan_t scanner;
    typedlualex_init(&scanner);
    auto buffer = typedlua_scan_bytes(source.data(), source.length(), scanner);
    
    typedlua::ast::Node* root = nullptr;

    std::ostringstream oss;

    if (typedluaparse(scanner, root) == 0) {
        if (root) {
            root->dump(oss, "");
        }
    }

    typedlua_delete_buffer(buffer, scanner);
    typedlualex_destroy(scanner);

    return oss.str();
}
