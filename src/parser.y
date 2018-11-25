%define parse.error verbose

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
    NExpr* expr;
    NArgSeq* argseq;
}

%destructor { delete $$; } <string>
%destructor { delete $$; } <node>
%destructor { delete $$; } <block>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <argseq>

%token <string> TIDENTIFIER TNUMBER TSTRING
%token <token> TEQUAL
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLSQBR TRSQBR
%token <token> TDOT TCOMMA TCOLON TSEMICOLON
%token <token> TPLUS TMINUS TSTAR TSLASH TSLASH2 TDOT2
%token <token> TDOT3

%type <node> stat assign
%type <block> block
%type <expr> expr var functioncall prefixexpr
%type <argseq> argseq args

%left TPLUS TMINUS
%left TSTAR TSLASH TSLASH2
%left TRPAREN
%left TLPAREN

%start chunk

%parse-param {Node*& root}

%%

chunk : block { root = $1; }
      ;

block : stat {
          $$ = new NBlock();
          $$->children.push_back(std::unique_ptr<Node>($1));
      }
      | block stat {
          $$ = $1;
          $$->children.push_back(std::unique_ptr<Node>($2));
      }
      ;

prefixexpr : var { $$ = $1; }
           | functioncall { $$ = $1; }
           | TLPAREN expr TRPAREN { $$ = $2; }
           ;

stat : TSEMICOLON { $$ = new NEmpty(); }
     | assign { $$ = $1; }
     | functioncall { $$ = $1; }
     ;

assign : var TEQUAL expr {
           $$ = new NAssignment(
               std::unique_ptr<NExpr>($1),
               std::unique_ptr<NExpr>($3));
       }
       ;

functioncall : prefixexpr args {
                 $$ = new NFunctionCall(
                     std::unique_ptr<NExpr>($1),
                     std::unique_ptr<NArgSeq>($2));
             }
             ;

expr : TNUMBER { $$ = new NNumberLiteral(std::move(*$1)); delete $1; }
     | prefixexpr %prec TRPAREN { $$ = $1; }
     ;

var : TIDENTIFIER { $$ = new NIdent(std::move(*$1)); delete $1; }
    | prefixexpr TLSQBR expr TRSQBR {
        $$ = new NSubscript(
            std::unique_ptr<NExpr>($1),
            std::unique_ptr<NExpr>($3));
    }
    | prefixexpr TDOT TIDENTIFIER {
        $$ = new NTableAccess(
            std::unique_ptr<NExpr>($1),
            std::move(*$3));
        delete $3;
    }
    ;

argseq : expr {
           $$ = new NArgSeq();
           $$->args.push_back(std::unique_ptr<NExpr>($1));
       }
       | argseq TCOMMA expr {
           $$ = $1;
           $$->args.push_back(std::unique_ptr<NExpr>($3));
       }
       ;

args : TLPAREN TRPAREN { $$ = nullptr; }
     | TLPAREN argseq TRPAREN { $$ = $2; }
     ;

%%
