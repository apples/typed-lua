cmake_minimum_required(VERSION 3.12)
project(TypedLua)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

flex_target(lexer src/lexer.l "${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp")
bison_target(parser src/parser.y "${CMAKE_CURRENT_BINARY_DIR}/parser.cpp")
add_flex_bison_dependency(lexer parser)

add_executable(typedlua src/main.cpp ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS})
set_target_properties(typedlua PROPERTIES CXX_STANDARD 17)
target_include_directories(typedlua PUBLIC src)
target_compile_definitions(typedlua PUBLIC $<$<CONFIG:Debug>:YYDEBUG=1>)
