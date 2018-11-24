%{
#include "node.hpp"
#include <string>
#include <iostream>

extern int yylex();
void yyerror(const char *s) { printf("ERROR: %sn", s); }
%}

%union {
    std::string* string;
    int token;
}

%destructor { delete $$; } <string>

%token <string> TIDENTIFIER TNUMBER TSTRING
%token <token> TEQUAL
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLSQBR TRSQBR
%token <token> TDOT TCOMMA TCOLON TSEMICOLON
%token <token> TPLUS TMINUS TSTAR TSLASH TSLASH2 TDOT2
%token <token> TDOT3

%type <token> program

%left TPLUS TMINUS
%left TSTAR TSLASH TSLASH2

%start program

%%

program : TNUMBER { std::cout << "number" << std::endl; }
        ;

%%
