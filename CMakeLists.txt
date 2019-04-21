cmake_minimum_required(VERSION 3.12)
project(TypedLua)

find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)
find_package(Lua REQUIRED)

flex_target(lexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp
    COMPILE_FLAGS "--header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.hpp"
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.hpp)
bison_target(parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.hpp)
add_flex_bison_dependency(lexer parser)

add_library(typedlua
    ${BISON_parser_OUTPUTS}
    ${FLEX_lexer_OUTPUTS}
    src/compile_error.hpp
    src/libs_basic.cpp
    src/libs_io.cpp
    src/libs_math.cpp
    src/libs_package.cpp
    src/libs_string.cpp
    src/libs_table.cpp
    src/libs.hpp
    src/loader.hpp
    src/loader.cpp
    src/node.hpp
    src/node.cpp
    src/require.hpp
    src/require.cpp
    src/type.hpp
    src/type.cpp
    src/typedlua_compiler.cpp
    src/typedlua_compiler.hpp)
set_target_properties(typedlua PROPERTIES CXX_STANDARD 17)
target_include_directories(typedlua PUBLIC src ${CMAKE_CURRENT_BINARY_DIR})
target_compile_definitions(typedlua PUBLIC $<$<CONFIG:Debug>:YYDEBUG=1>)
target_include_directories(typedlua PUBLIC ${LUA_INCLUDE_DIR})
target_link_libraries(typedlua PUBLIC ${LUA_LIBRARIES})

add_executable(tlc src/main.cpp)
set_target_properties(tlc PROPERTIES CXX_STANDARD 17)
target_link_libraries(tlc typedlua)

function(add_example name)
    add_executable(example_${name} examples/${name}.cpp)
    set_target_properties(example_${name} PROPERTIES CXX_STANDARD 17)
    target_link_libraries(example_${name} typedlua)
endfunction()

add_example(simple)
add_example(require)
