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
    std::vector<std::unique_ptr<NElseIf>>* elseifseq;
    NElseIf* elseif;
    NElse* else_;
}

%destructor { delete $$; } <string>
%destructor { delete $$; } <node>
%destructor { delete $$; } <block>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <argseq>
%destructor { delete $$; } <elseifseq>
%destructor { delete $$; } <elseif>
%destructor { delete $$; } <else_>

%token <string> TIDENTIFIER TNUMBER TSTRING
%token <token> TEQUAL
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TLSQBR TRSQBR
%token <token> TDOT TCOMMA TCOLON TSEMICOLON
%token <token> TPLUS TMINUS TSTAR TSLASH TSLASH2 TDOT2
%token <token> TDOT3
%token <token> TCOLON2
%token <token> TKAND TKBREAK TKDO TKELSE TKELSEIF TKEND
%token <token> TKFALSE TKFOR TKFUNCTION TKGOTO TKIF TKIN
%token <token> TKLOCAL TKNIL TKNOT TKOR TKREPEAT TKRETURN
%token <token> TKTHEN TKTRUE TKUNTIL TKWHILE

%type <node> stat assign label goto while repeat if foriter
%type <block> block
%type <expr> expr var functioncall prefixexpr
%type <argseq> argseq args
%type <elseifseq> elseifseq
%type <elseif> elseif
%type <else_> else

%left TPLUS TMINUS
%left TSTAR TSLASH TSLASH2
%left TRPAREN
%left TLPAREN

%start chunk

%parse-param {Node*& root}

%%

chunk: block { root = $1; }
     ;

block: stat {
         $$ = new NBlock();
         $$->children.push_back(std::unique_ptr<Node>($1));
     }
     | block stat {
         $$ = $1;
         $$->children.push_back(std::unique_ptr<Node>($2));
     }
     ;

prefixexpr: var { $$ = $1; }
          | functioncall { $$ = $1; }
          | TLPAREN expr TRPAREN { $$ = $2; }
          ;

stat: TSEMICOLON { $$ = new NEmpty(); }
    | assign { $$ = $1; }
    | functioncall { $$ = $1; }
    | label { $$ = $1; }
    | TKBREAK { $$ = new NBreak(); }
    | goto { $$ = $1; }
    | TKDO block TKEND { $$ = $2; }
    | while { $$ = $1; }
    | repeat { $$ = $1; }
    | if { $$ = $1; }
    | foriter { $$ = $1; }
    ;

foriter: TKFOR TIDENTIFIER TEQUAL expr TCOMMA expr TCOMMA expr TKDO block TKEND {
           $$ = new NForIter(
               std::move(*$2),
               std::unique_ptr<NExpr>($4),
               std::unique_ptr<NExpr>($6),
               std::unique_ptr<NExpr>($8),
               std::unique_ptr<NBlock>($10));
           delete $2;
       }
       | TKFOR TIDENTIFIER TEQUAL expr TCOMMA expr TKDO block TKEND {
           $$ = new NForIter(
               std::move(*$2),
               std::unique_ptr<NExpr>($4),
               std::unique_ptr<NExpr>($6),
               std::unique_ptr<NExpr>(nullptr),
               std::unique_ptr<NBlock>($8));
           delete $2;
       }
       ;

if: TKIF expr TKTHEN block elseifseq else TKEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>($6));
      delete $5;
  }
  | TKIF expr TKTHEN block elseifseq TKEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>(nullptr));
      delete $5;
  }
  | TKIF expr TKTHEN block else TKEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          {},
          std::unique_ptr<NElse>($5));
  }
  | TKIF expr TKTHEN block TKEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          {},
          std::unique_ptr<NElse>(nullptr));
  }
  ;

elseifseq: elseif {
             $$ = new std::vector<std::unique_ptr<NElseIf>>();
             $$->push_back(std::unique_ptr<NElseIf>($1));
         }
         | elseifseq elseif {
             $$ = $1;
             $$->push_back(std::unique_ptr<NElseIf>($2));
         }
         ;

elseif: TKELSEIF expr TKTHEN block {
          $$ = new NElseIf(
              std::unique_ptr<NExpr>($2), std::unique_ptr<NBlock>($4));
      }
      ;

else: TKELSE block { $$ = new NElse(std::unique_ptr<NBlock>($2)); }
    ;

repeat: TKREPEAT block TKUNTIL expr {
          $$ = new NRepeat(
              std::unique_ptr<NBlock>($2),
              std::unique_ptr<NExpr>($4));
      }
      ;

while: TKWHILE expr TKDO block TKEND {
         $$ = new NWhile(
             std::unique_ptr<NExpr>($2),
             std::unique_ptr<NBlock>($4));
     }
     ;

goto: TKGOTO TIDENTIFIER {
        $$ = new NGoto(std::move(*$2));
        delete $2;
    }
    ;

label: TCOLON2 TIDENTIFIER TCOLON2 {
         $$ = new NLabel(std::move(*$2));
         delete $2;
     }
     ;

assign: var TEQUAL expr {
          $$ = new NAssignment(
              std::unique_ptr<NExpr>($1),
              std::unique_ptr<NExpr>($3));
      }
      ;

functioncall: prefixexpr args {
                $$ = new NFunctionCall(
                    std::unique_ptr<NExpr>($1),
                    std::unique_ptr<NArgSeq>($2));
            }
            ;

expr: TNUMBER { $$ = new NNumberLiteral(std::move(*$1)); delete $1; }
    | prefixexpr %prec TRPAREN { $$ = $1; }
    ;

var: TIDENTIFIER { $$ = new NIdent(std::move(*$1)); delete $1; }
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

argseq: expr {
          $$ = new NArgSeq();
          $$->args.push_back(std::unique_ptr<NExpr>($1));
      }
      | argseq TCOMMA expr {
          $$ = $1;
          $$->args.push_back(std::unique_ptr<NExpr>($3));
      }
      ;

args: TLPAREN TRPAREN { $$ = nullptr; }
    | TLPAREN argseq TRPAREN { $$ = $2; }
    ;

%%
