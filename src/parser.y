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
    std::string* string;
    Node* node;
    NBlock* block;
    NExpr* expr;
    NArgSeq* argseq;
    std::vector<std::unique_ptr<NElseIf>>* elseifseq;
    NElseIf* elseif;
    NElse* else_;
    std::vector<std::string>* strings;
    std::vector<std::unique_ptr<NExpr>>* exprs;
}

%destructor { delete $$; } <string>
%destructor { delete $$; } <node>
%destructor { delete $$; } <block>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <argseq>
%destructor { delete $$; } <elseifseq>
%destructor { delete $$; } <elseif>
%destructor { delete $$; } <else_>
%destructor { delete $$; } <strings>
%destructor { delete $$; } <exprs>

%token TCEQ TCNE TCLE TCGE
%token TSLASH2 TDOT2 TDOT3 TCOLON2
%token TAND TBREAK TDO TELSE TELSEIF TEND
%token TFALSE TFOR TFUNCTION TGOTO TIF TIN
%token TLOCAL TNIL TNOT TOR TREPEAT TRETURN
%token TTHEN TTRUE TUNTIL TWHILE

%token <string> TIDENTIFIER TNUMBER TSTRING

%left '+' '-'
%left '*' '/' TSLASH2
%left ')'
%left '('

%type <node> stat assign label goto while repeat if fornumeric forgeneric
%type <node> function localfunc retstat localvar
%type <block> block statseq
%type <expr> expr var functioncall prefixexpr funcvar functiondef
%type <argseq> argseq args
%type <elseifseq> elseifseq
%type <elseif> elseif
%type <else_> else
%type <strings> namelist funcparams
%type <exprs> explist varlist

%start chunk

%parse-param {Node*& root}

%%

chunk: block { root = $1; }
     ;

block: statseq { $$ = $1; }
     | statseq retstat {
         $$ = $1;
         $$->children.push_back(std::unique_ptr<Node>($retstat));
     }
     | retstat {
         $$ = new NBlock();
         $$->children.push_back(std::unique_ptr<Node>($retstat));
     }
     ;

statseq: stat {
           $$ = new NBlock();
           $$->children.push_back(std::unique_ptr<Node>($1));
       }
       | statseq stat {
           $$ = $1;
           $$->children.push_back(std::unique_ptr<Node>($2));
       }
       ;

retstat: TRETURN { $$ = new NReturn(); }
       | TRETURN explist {
           $$ = new NReturn(std::move(*$explist));
           delete $explist;
       }
       | retstat ';' { $$ = $1; }
       ;

prefixexpr: var { $$ = $1; }
          | functioncall { $$ = $1; }
          | '(' expr ')' { $$ = $2; }
          ;

namelist: TIDENTIFIER {
            $$ = new std::vector<std::string>();
            $$->push_back(std::move(*$1));
            delete $1;
        }
        | namelist ',' TIDENTIFIER {
            $$ = $1;
            $$->push_back(std::move(*$3));
            delete $3;
        }
        ;

stat: ';' { $$ = new NEmpty(); }
    | assign { $$ = $1; }
    | functioncall { $$ = $1; }
    | label { $$ = $1; }
    | TBREAK { $$ = new NBreak(); }
    | goto { $$ = $1; }
    | TDO block TEND { $$ = $2; }
    | while { $$ = $1; }
    | repeat { $$ = $1; }
    | if { $$ = $1; }
    | fornumeric { $$ = $1; }
    | forgeneric { $$ = $1; }
    | function { $$ = $1; }
    | localfunc { $$ = $1; }
    | localvar { $$ = $1; }
    ;

localvar: TLOCAL namelist {
            $$ = new NLocalVar(
                std::move(*$namelist),
                {});
            delete $namelist;
        }
        | TLOCAL namelist '=' explist {
            $$ = new NLocalVar(
                std::move(*$namelist),
                std::move(*$explist));
            delete $namelist;
            delete $explist;
        }
        ;

localfunc: TLOCAL TFUNCTION TIDENTIFIER[name] funcparams block TEND {
             $$ = new NLocalFunction(
                 std::move(*$name),
                 std::move(*$funcparams),
                 std::unique_ptr<NBlock>($block));
             delete $name;
             delete $funcparams;
         }
         ;

function: TFUNCTION funcvar funcparams block TEND {
            $$ = new NFunction(
                std::unique_ptr<NExpr>($funcvar),
                std::move(*$funcparams),
                std::unique_ptr<NBlock>($block));
            delete $funcparams;
        }
        | TFUNCTION funcvar ':' TIDENTIFIER[name] funcparams block TEND {
            $$ = new NSelfFunction(
                std::move(*$name),
                std::unique_ptr<NExpr>($funcvar),
                std::move(*$funcparams),
                std::unique_ptr<NBlock>($block));
            delete $name;
            delete $funcparams;
        }
        ;

funcvar: TIDENTIFIER { $$ = new NIdent(std::move(*$1)); delete $1; }
       | funcvar '.' TIDENTIFIER {
           $$ = new NTableAccess(
               std::unique_ptr<NExpr>($1),
               std::move(*$3));
           delete $3;
       }
       ;

funcparams: '(' ')' { $$ = new std::vector<std::string>(); }
          | '(' namelist ')' { $$ = $namelist; }
          ;

forgeneric: TFOR namelist TIN explist TDO block TEND {
           $$ = new NForGeneric(
               std::move(*$namelist),
               std::move(*$explist),
               std::unique_ptr<NBlock>($block));
           delete $namelist;
           delete $explist;
       }
       ;

fornumeric: TFOR TIDENTIFIER '=' expr ',' expr ',' expr TDO block TEND {
              $$ = new NForNumeric(
                  std::move(*$2),
                  std::unique_ptr<NExpr>($4),
                  std::unique_ptr<NExpr>($6),
                  std::unique_ptr<NExpr>($8),
                  std::unique_ptr<NBlock>($10));
              delete $2;
          }
          | TFOR TIDENTIFIER '=' expr ',' expr TDO block TEND {
              $$ = new NForNumeric(
                  std::move(*$2),
                  std::unique_ptr<NExpr>($4),
                  std::unique_ptr<NExpr>($6),
                  std::unique_ptr<NExpr>(nullptr),
                  std::unique_ptr<NBlock>($8));
              delete $2;
          }
          ;

if: TIF expr TTHEN block elseifseq else TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>($6));
      delete $5;
  }
  | TIF expr TTHEN block elseifseq TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>(nullptr));
      delete $5;
  }
  | TIF expr TTHEN block else TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          {},
          std::unique_ptr<NElse>($5));
  }
  | TIF expr TTHEN block TEND {
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

elseif: TELSEIF expr TTHEN block {
          $$ = new NElseIf(
              std::unique_ptr<NExpr>($2), std::unique_ptr<NBlock>($4));
      }
      ;

else: TELSE block { $$ = new NElse(std::unique_ptr<NBlock>($2)); }
    ;

repeat: TREPEAT block TUNTIL expr {
          $$ = new NRepeat(
              std::unique_ptr<NBlock>($2),
              std::unique_ptr<NExpr>($4));
      }
      ;

while: TWHILE expr TDO block TEND {
         $$ = new NWhile(
             std::unique_ptr<NExpr>($2),
             std::unique_ptr<NBlock>($4));
     }
     ;

goto: TGOTO TIDENTIFIER {
        $$ = new NGoto(std::move(*$2));
        delete $2;
    }
    ;

label: TCOLON2 TIDENTIFIER TCOLON2 {
         $$ = new NLabel(std::move(*$2));
         delete $2;
     }
     ;

assign: varlist '=' explist {
          $$ = new NAssignment(
              std::move(*$varlist),
              std::move(*$explist));
      }
      ;

functioncall: prefixexpr args {
                $$ = new NFunctionCall(
                    std::unique_ptr<NExpr>($1),
                    std::unique_ptr<NArgSeq>($2));
            }
            ;

expr: TNIL { $$ = new NNil(); }
    | TFALSE { $$ = new NBooleanLiteral(false); }
    | TTRUE { $$ = new NBooleanLiteral(true); }
    | TNUMBER { $$ = new NNumberLiteral(std::move(*$1)); delete $1; }
    | TSTRING { $$ = new NStringLiteral(std::move(*$1)); delete $1; }
    | TDOT3 { $$ = new NDots(); }
    | functiondef { $$ = $1; }
    | prefixexpr %prec ')' { $$ = $1; }
    ;

functiondef: TFUNCTION funcparams block TEND {
               $$ = new NFunctionDef(
                   std::move(*$funcparams),
                   std::unique_ptr<NBlock>($block));
               delete $funcparams;
           }
           ;

explist: expr {
           $$ = new std::vector<std::unique_ptr<NExpr>>();
           $$->push_back(std::unique_ptr<NExpr>($1));
       }
       | explist ',' expr {
           $$ = $1;
           $$->push_back(std::unique_ptr<NExpr>($3));
       }
       ;

var: TIDENTIFIER { $$ = new NIdent(std::move(*$1)); delete $1; }
   | prefixexpr '[' expr ']' {
       $$ = new NSubscript(
           std::unique_ptr<NExpr>($1),
           std::unique_ptr<NExpr>($3));
   }
   | prefixexpr '.' TIDENTIFIER {
       $$ = new NTableAccess(
           std::unique_ptr<NExpr>($1),
           std::move(*$3));
       delete $3;
   }
   ;

varlist: var {
           $$ = new std::vector<std::unique_ptr<NExpr>>();
           $$->push_back(std::unique_ptr<NExpr>($var));
       }
       | varlist ',' var {
           $$ = $1;
           $$->push_back(std::unique_ptr<NExpr>($var));
       }
       ;

argseq: expr {
          $$ = new NArgSeq();
          $$->args.push_back(std::unique_ptr<NExpr>($1));
      }
      | argseq ',' expr {
          $$ = $1;
          $$->args.push_back(std::unique_ptr<NExpr>($3));
      }
      ;

args: '(' ')' { $$ = nullptr; }
    | '(' argseq ')' { $$ = $2; }
    ;

%%
