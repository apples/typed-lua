#include <iostream>
#include <sstream>

#include "typedlua_compiler.hpp"

int main(int argc, char **argv) {
    auto tlc = typedlua_compiler();
    
    std::stringstream ss;
    ss << std::cin.rdbuf();

    std::cout << tlc.compile(ss.str(), "cin");
}
