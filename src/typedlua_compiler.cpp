#include "typedlua_compiler.hpp"

#include "parser.hpp"
#include "lexer.hpp"

#include <sstream>

namespace typedlua {

Compiler::Compiler() {}

Compiler::Compiler(Scope& global_scope) : global_scope(&global_scope) {}

auto Compiler::compile(std::string_view source, std::string_view name) -> Result {
    yyscan_t scanner;
    typedlualex_init(&scanner);
    auto buffer = typedlua_scan_bytes(source.data(), source.length(), scanner);
    typedluaset_lineno(1, scanner);
    
    typedlua::ast::Node* root = nullptr;

    std::ostringstream oss;
    auto rv = Result{};

    if (typedluaparse(scanner, root) == 0 && root) {
        auto scope = Scope(global_scope);
        std::vector<CompileError> errors;
        root->check(scope, errors);
        oss << *root << std::endl;

        rv.new_source = oss.str();
        rv.errors = std::move(errors);
    }

    typedlua_delete_buffer(buffer, scanner);
    typedlualex_destroy(scanner);

    return rv;
}

} // namespace typedlua
