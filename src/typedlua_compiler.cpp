#include "typedlua_compiler.hpp"

#include "parser.hpp"
#include "lexer.hpp"
#include "node.hpp"

#include <sstream>

namespace typedlua {

std::tuple<std::unique_ptr<ast::Node>, std::vector<CompileError>> parse(std::string_view source) {
    yyscan_t scanner;

    typedlualex_init(&scanner);
    
    auto buffer = typedlua_scan_bytes(source.data(), source.length(), scanner);
    
    typedluaset_lineno(1, scanner);
    
    auto root = std::unique_ptr<ast::Node>{};

    auto errors = std::vector<CompileError>{};

    if (typedluaparse(scanner, root, errors) != 0) {
        root = nullptr;
    }

    typedlua_delete_buffer(buffer, scanner);
    typedlualex_destroy(scanner);

    return {std::move(root), std::move(errors)};
}

std::vector<CompileError> check(const ast::Node& root, Scope& scope) {
    auto errors = std::vector<CompileError>{};

    root.check(scope, errors);

    return errors;
}

std::string compile(const ast::Node& root) {
    auto oss = std::ostringstream{};

    oss << root << std::endl;

    return oss.str();
}

} // namespace typedlua
