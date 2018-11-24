%code top {
    extern int yylex();
}

%code requires {
    #include "node.hpp"
    #include <iostream>
    #include <string>

    using namespace typedlua::ast;

    static void yyerror(Node* root, const char *s) {
        std::cerr << "ERROR: " << s << "\n";
    }
}

%union {
    int token;
    std::string* string;
    Node* node;
    NBlock* block;
}

%destructor { delete $$; } <string>
%destructor { delete $$; } <node>

%token <string> TIDENTIFIER TNUMBER TSTRING
%token <token> TEQUAL
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLSQBR TRSQBR
%token <token> TDOT TCOMMA TCOLON TSEMICOLON
%token <token> TPLUS TMINUS TSTAR TSLASH TSLASH2 TDOT2
%token <token> TDOT3

%type <node> stmt
%type <block> block

%left TPLUS TMINUS
%left TSTAR TSLASH TSLASH2

%start program

%parse-param {Node*& root}

%%

program : block { root = $1; }
        ;

block : stmt {
            $$ = new NBlock();
            $$->children.push_back(std::unique_ptr<Node>($1));
        }
      | block stmt { $1->children.push_back(std::unique_ptr<Node>($2)); }
      ;

stmt : TNUMBER { $$ = new NNumberLiteral(std::move(*$1)); delete $1; }
     ;

%%
