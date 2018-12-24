#include "typedlua_compiler.hpp"

#include "parser.hpp"
#include "lexer.hpp"
#include "node.hpp"

#include <sstream>

namespace typedlua {

std::unique_ptr<ast::Node> parse(std::string_view source) {
    yyscan_t scanner;

    typedlualex_init(&scanner);
    
    auto buffer = typedlua_scan_bytes(source.data(), source.length(), scanner);
    
    typedluaset_lineno(1, scanner);
    
    auto root = std::unique_ptr<ast::Node>{};

    if (typedluaparse(scanner, root) != 0) {
        root = nullptr;
    }

    typedlua_delete_buffer(buffer, scanner);
    typedlualex_destroy(scanner);

    return root;
}

std::vector<CompileError> check(const ast::Node& root, Scope& global_scope) {
    auto scope = Scope(global_scope);
    auto errors = std::vector<CompileError>{};

    scope.deduce_return_type();
    root.check(scope, errors);

    return errors;
}

std::string compile(const ast::Node& root) {
    auto oss = std::ostringstream{};

    oss << root << std::endl;

    return oss.str();
}

} // namespace typedlua
