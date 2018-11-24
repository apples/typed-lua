#include <iostream>
#include "node.hpp"
extern int yyparse();

int main(int argc, char **argv)
{
    yyparse();
}
