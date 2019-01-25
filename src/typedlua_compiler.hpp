#pragma once

#include "compile_error.hpp"
#include "scope.hpp"
#include "node.hpp"

#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace typedlua {

std::tuple<std::unique_ptr<ast::Node>, std::vector<CompileError>> parse(std::string_view source);

std::vector<CompileError> check(const ast::Node& root, Scope& scope);

std::string compile(const ast::Node& root);

} // namespace typedlua
