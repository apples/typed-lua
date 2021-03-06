/// Parser grammar

%define api.prefix {typedlua}
%define parse.error verbose

// Defines a reentrant parser and lexer.
// The first parse-param should be a `yyscan_t`,
// but due to a circular dependency, this seems impossible.
%define api.pure full
%lex-param {yyscan_t scanner}
%parse-param {void* scanner} {std::unique_ptr<typedlua::ast::Node>& root} {std::vector<typedlua::CompileError>& errors}

%locations

%code requires {
    #include "node.hpp"
    #include "location.hpp"
    #include <string>

    using TYPEDLUALTYPE = typedlua::Location;
    #define TYPEDLUALTYPE_IS_DECLARED 1
    #define TYPEDLUALTYPE_IS_TRIVIAL 1
}

%union {
    std::string* string;
    typedlua::ast::Node* node;
    typedlua::ast::NBlock* block;
    typedlua::ast::NExpr* expr;
    typedlua::ast::NArgSeq* argseq;
    std::vector<std::unique_ptr<typedlua::ast::NElseIf>>* elseifseq;
    typedlua::ast::NElseIf* elseif;
    typedlua::ast::NElse* else_;
    std::vector<std::unique_ptr<typedlua::ast::NExpr>>* exprs;
    typedlua::ast::NField* field;
    std::vector<std::unique_ptr<typedlua::ast::NField>>* fields;
    typedlua::ast::NFuncParams* params;
    std::vector<typedlua::ast::NNameDecl>* namedecls;
    typedlua::ast::NType* type;
    std::vector<typedlua::ast::NTypeFunctionParam>* typefuncparams;
    typedlua::ast::NIndex* index;
    typedlua::ast::NIndexList* indexlist;
    typedlua::ast::NFieldDecl* fielddecl;
    typedlua::ast::NFieldDeclList* fielddecllist;
    typedlua::ast::NTypeFunction* typefunction;
    std::vector<std::unique_ptr<typedlua::ast::NType>>* types;
}

%code {
    #include "lexer.hpp"
    #include "compile_error.hpp"

    using namespace typedlua::ast;

    static void yyerror(
        YYLTYPE* loc,
        void* scanner,
        std::unique_ptr<Node>& root,
        std::vector<typedlua::CompileError>& errors,
        const char *s)
    {
        errors.emplace_back(s, *loc);
    }

    template <typename T>
    std::unique_ptr<T> ptr(T* expr) {
        return std::unique_ptr<T>(expr);
    }

    #define BINOP(OBJ, OP, LEFT, RIGHT, LOC) \
        OBJ = new NBinop(NBinop::Op::OP, ptr(LEFT), ptr(RIGHT)); \
        OBJ->location = LOC;
    #define UNARYOP(OBJ, OP, EXPR, LOC) \
        OBJ = new NUnaryop(NUnaryop::Op::OP, ptr(EXPR)); \
        OBJ->location = LOC;
}

%destructor { delete $$; } <string>
%destructor { delete $$; } <node>
%destructor { delete $$; } <block>
%destructor { delete $$; } <expr>
%destructor { delete $$; } <argseq>
%destructor { delete $$; } <elseifseq>
%destructor { delete $$; } <elseif>
%destructor { delete $$; } <else_>
%destructor { delete $$; } <exprs>
%destructor { delete $$; } <field>
%destructor { delete $$; } <fields>
%destructor { delete $$; } <params>
%destructor { delete $$; } <namedecls>
%destructor { delete $$; } <type>
%destructor { delete $$; } <typefuncparams>
%destructor { delete $$; } <index>
%destructor { delete $$; } <indexlist>
%destructor { delete $$; } <fielddecl>
%destructor { delete $$; } <fielddecllist>
%destructor { delete $$; } <typefunction>

%token TCEQ TCNE TCLE TCGE TSHL TSHR
%token TSLASH2 TDOT2 TDOT3 TCOLON2
%token TAND TBREAK TDO TELSE TELSEIF TEND
%token TFALSE TFOR TFUNCTION TGOTO TIF TIN
%token TLOCAL TNIL TNOT TOR TREPEAT TRETURN
%token TTHEN TTRUE TUNTIL TWHILE

%token TGLOBAL TINTERFACE T_REQUIRE

%token <string> TIDENTIFIER TNUMBER TSTRING

%left TOR
%left TAND
%left '<' '>' TCLE TCGE TCNE TCEQ
%left '|'
%left '~'
%left '&'
%left TSHL TSHR
%right TDOT2
%left '+' '-'
%left '*' '/' TSLASH2 '%'
%left TNOT '#' NEG BNOT
%right '^'
%left ')'
%left '('

%type <node> stat assign label goto while repeat if fornumeric forgeneric
%type <node> function localfunc retstat localvar globalvar interface
%type <block> block statseq
%type <expr> expr var functioncall prefixexpr funcvar functiondef
%type <expr> tableconstructor binopcall unaryopcall
%type <argseq> argseq args
%type <elseifseq> elseifseq
%type <elseif> elseif
%type <else_> else
%type <exprs> explist varlist
%type <field> field
%type <fields> fieldlist
%type <params> funcparams
%type <namedecls> namelist funcgenparams
%type <type> commonunittype unittype type typetuple idtype tabletype
%type <type> funcret rettype functype retunittype literaltype
%type <type> requiretype genericcalltype
%type <typefuncparams> typefuncparams
%type <index> index
%type <indexlist> indexes tableindexes
%type <fielddecl> fielddecl
%type <fielddecllist> fielddecls tablefields
%type <typefunction> regfunctype
%type <types> typelist

%start chunk

%%

chunk: block { root = ptr($1); }
     ;

block: statseq { $$ = $1; }
     | statseq retstat {
         $$ = $1;
         $$->location = @$;
         $$->children.push_back(std::unique_ptr<Node>($retstat));
     }
     | retstat {
         $$ = new NBlock();
         $$->location = @$;
         $$->children.push_back(std::unique_ptr<Node>($retstat));
     }
     | %empty {
         $$ = new NBlock();
         $$->location = @$;
     }
     ;

statseq: stat {
           $$ = new NBlock();
           $$->location = @$;
           $$->children.push_back(std::unique_ptr<Node>($1));
       }
       | statseq stat {
           $$ = $1;
           $$->location = @$;
           $$->children.push_back(std::unique_ptr<Node>($2));
       }
       ;

retstat: TRETURN { $$ = new NReturn(); $$->location = @$; }
       | TRETURN explist {
           $$ = new NReturn(std::move(*$explist));
           $$->location = @$;
           delete $explist;
       }
       | retstat ';' { $$ = $1; $$->location = @$; }
       ;

prefixexpr: var { $$ = $1; }
          | functioncall { $$ = $1; }
          | '(' expr ')' { $$ = $2; $$->location = @$; }
          ;

expr: TNIL { $$ = new NNil(); }
    | TFALSE { $$ = new NBooleanLiteral(false); }
    | TTRUE { $$ = new NBooleanLiteral(true); }
    | TNUMBER { $$ = new NNumberLiteral(std::move(*$1)); delete $1; }
    | TSTRING { $$ = new NStringLiteral(std::move(*$1)); delete $1; }
    | TDOT3 { $$ = new NDots(); }
    | functiondef { $$ = $1; }
    | prefixexpr %prec ')' { $$ = $1; }
    | tableconstructor { $$ = $1; }
    | binopcall { $$ = $1; }
    | unaryopcall { $$ = $1; }
    ;

stat: ';' { $$ = new NEmpty(); $$->location = @$; }
    | assign { $$ = $1; }
    | functioncall { $$ = $1; }
    | label { $$ = $1; }
    | TBREAK { $$ = new NBreak(); $$->location = @$; }
    | goto { $$ = $1; }
    | TDO block TEND { $$ = $2; $2->scoped = true; $$->location = @$; }
    | while { $$ = $1; }
    | repeat { $$ = $1; }
    | if { $$ = $1; }
    | fornumeric { $$ = $1; }
    | forgeneric { $$ = $1; }
    | function { $$ = $1; }
    | localfunc { $$ = $1; }
    | localvar { $$ = $1; }
    | globalvar { $$ = $1; }
    | interface { $$ = $1; }
    ;

namelist: TIDENTIFIER {
            $$ = new std::vector<NNameDecl>();
            $$->emplace_back(std::move(*$1));
            $$->back().location = @$;
            delete $1;
        }
        | TIDENTIFIER ':' type {
            $$ = new std::vector<NNameDecl>();
            $$->emplace_back(std::move(*$1), std::unique_ptr<NType>($type));
            $$->back().location = @$;
            delete $1;
        }
        | namelist ',' TIDENTIFIER {
            $$ = $1;
            $$->emplace_back(std::move(*$3));
            $$->back().location = @3;
            delete $3;
        }
        | namelist ',' TIDENTIFIER ':' type {
            $$ = $1;
            $$->emplace_back(std::move(*$3), std::unique_ptr<NType>($type));
            $$->back().location = @3;
            $$->back().location.last_line = @5.last_line;
            $$->back().location.last_column = @5.last_column;
            delete $3;
        }
        ;

type: unittype { $$ = $1; }
    | functype { $$ = $1; }
    ;

idtype: TIDENTIFIER {
          $$ = new NTypeName(std::move(*$1));
          $$->location = @$;
          delete $1;
      }
      | TNIL {
          $$ = new NTypeName("nil");
          $$->location = @$;
      }
      ;

commonunittype: idtype { $$ = $1; }
              | '(' type ')' { $$ = $2; $$->location = @$; }
              | tabletype { $$ = $1; }
              | literaltype { $$ = $1; }
              | requiretype { $$ = $1; }
              | genericcalltype { $$ = $1; }
              ;

unittype: commonunittype { $$ = $1; }
        | unittype[lhs] '|' unittype[rhs] {
            $$ = new NTypeSum(ptr($lhs), ptr($rhs));
            $$->location = @$;
        }
        | unittype[lhs] '&' unittype[rhs] {
            $$ = new NTypeProduct(ptr($lhs), ptr($rhs));
            $$->location = @$;
        }
        ;

literaltype: TFALSE { $$ = new NTypeLiteralBoolean(false); $$->location = @$; }
           | TTRUE { $$ = new NTypeLiteralBoolean(true); $$->location = @$; }
           | TNUMBER {
               $$ = new NTypeLiteralNumber(*$1);
               $$->location = @$;
               delete $1;
           }
           | TSTRING {
               $$ = new NTypeLiteralString(*$1);
               $$->location = @$;
               delete $1;
           }
           ;

requiretype: T_REQUIRE '(' type ')' { $$ = new NTypeRequire(ptr($type)); $$->location = @$; }
           ;

genericcalltype: idtype '<' typelist '>' {
                   $$ = new NTypeGenericCall(ptr($idtype), std::move(*$typelist));
                   $$->location = @$;
                   delete $typelist;
               }
               ;

typelist: type {
            $$ = new std::vector<std::unique_ptr<NType>>{};
            $$->push_back(ptr($type));
        }
        | typelist ',' type { $$->push_back(ptr($type)); }
        ;

regfunctype: '(' ')' ':' rettype[ret] {
               $$ = new NTypeFunction({}, std::unique_ptr<NType>($ret), false);
               $$->location = @$;
           }
           | '(' typefuncparams ')' ':' rettype[ret] {
               $$ = new NTypeFunction(std::move(*$typefuncparams), ptr($ret), false);
               $$->location = @$;
               delete $typefuncparams;
           }
           | '(' TDOT3 ')' ':' rettype[ret] {
               $$ = new NTypeFunction({}, std::unique_ptr<NType>($ret), true);
               $$->location = @$;
           }
           | '(' typefuncparams ',' TDOT3 ')' ':' rettype[ret] {
               $$ = new NTypeFunction(std::move(*$typefuncparams), ptr($ret), true);
               $$->location = @$;
               delete $typefuncparams;
           }
           ;

functype: regfunctype { $$ = $1; }
        | '<' namelist '>' regfunctype {
            $regfunctype->generic_params = std::move(*$namelist);
            $$ = $regfunctype;
            $$->location = @$;
            delete $namelist;
        }
        ;

retunittype: commonunittype { $$ = $1; }
           | typetuple { $$ = $1; }
           | retunittype[lhs] '|' retunittype[rhs] {
               $$ = new NTypeSum(ptr($lhs), ptr($rhs));
               $$->location = @$;
           }
           ;

rettype: retunittype { $$ = $1; }
       | functype { $$ = $1; }
       ;

typetuple: '[' typefuncparams ']' {
             $$ = new NTypeTuple(std::move(*$typefuncparams), false);
             $$->location = @$;
             delete $typefuncparams;
         }
         | '[' typefuncparams ',' TDOT3 ']' {
             $$ = new NTypeTuple(std::move(*$typefuncparams), true);
             $$->location = @$;
             delete $typefuncparams;
         }
         | '[' TDOT3 ']' {
             $$ = new NTypeTuple({}, true);
             $$->location = @$;
         }
         ;

typefuncparams: ':' type {
                  $$ = new std::vector<NTypeFunctionParam>();
                  $$->emplace_back("", std::unique_ptr<NType>($type));
                  $$->back().location = @$;
              }
              | TIDENTIFIER[name] ':' type {
                  $$ = new std::vector<NTypeFunctionParam>();
                  $$->emplace_back(std::move(*$name), std::unique_ptr<NType>($type));
                  $$->back().location = @$;
                  delete $name;
              }
              | typefuncparams ',' ':'[colon] type {
                  $$ = $1;
                  $$->emplace_back("", std::unique_ptr<NType>($type));
                  $$->back().location = {
                      @colon.first_line,
                      @colon.first_column,
                      @type.last_line,
                      @type.last_column};
              }
              | typefuncparams ',' TIDENTIFIER[name] ':' type {
                  $$ = $1;
                  $$->emplace_back(std::move(*$name), std::unique_ptr<NType>($type));
                  $$->back().location = {
                      @name.first_line,
                      @name.first_column,
                      @type.last_line,
                      @type.last_column};
                  delete $name;
              }
              ;

tabletype: '{' tableindexes tablefields '}' {
             $$ = new NTypeTable(ptr($tableindexes), ptr($tablefields));
             $$->location = @$;
         }
         ;

tableindexes: %empty { $$ = nullptr; }
            | indexes { $$ = $1; }
            ;

indexes: index {
           $$ = new NIndexList();
           $$->location = @$;
           $$->indexes.push_back(ptr($index));
       }
       | indexes index {
           $$ = $1;
           $$->location = @$;
           $$->indexes.push_back(ptr($index));
       }
       | indexes ';' {
           $$ = $1;
           $$->location = @$;
       }
       ;

index: '[' type[key] ']' ':' type[val] {
         $$ = new NIndex(ptr($key), ptr($val));
         $$->location = @$;
     }
     ;

tablefields: %empty { $$ = nullptr; }
           | fielddecls { $$ = $1; }
           ;

fielddecls: fielddecl {
              $$ = new NFieldDeclList();
              $$->location = @$;
              $$->fields.push_back(ptr($fielddecl));
          }
          | fielddecls fielddecl {
              $$ = $1;
              $$->location = @$;
              $$->fields.push_back(ptr($fielddecl));
          }
          | fielddecls ';' {
              $$ = $1;
              $$->location = @$;
          }
          ;

fielddecl: TIDENTIFIER[name] ':' type {
             $$ = new NFieldDecl(std::move(*$name), ptr($type));
             $$->location = @$;
             delete $name;
         }
         ;

binopcall: expr[l] TOR expr[r] { BINOP($$, OR, $l, $r, @$) }
         | expr[l] TAND expr[r] { BINOP($$, AND, $l, $r, @$) }
         | expr[l] '<' expr[r] { BINOP($$, LT, $l, $r, @$) }
         | expr[l] '>' expr[r] { BINOP($$, GT, $l, $r, @$) }
         | expr[l] TCLE expr[r] { BINOP($$, LEQ, $l, $r, @$) }
         | expr[l] TCGE expr[r] { BINOP($$, GEQ, $l, $r, @$) }
         | expr[l] TCNE expr[r] { BINOP($$, NEQ, $l, $r, @$) }
         | expr[l] TCEQ expr[r] { BINOP($$, EQ, $l, $r, @$) }
         | expr[l] '|' expr[r] { BINOP($$, BOR, $l, $r, @$) }
         | expr[l] '~' expr[r] { BINOP($$, BXOR, $l, $r, @$) }
         | expr[l] '&' expr[r] { BINOP($$, BAND, $l, $r, @$) }
         | expr[l] TSHL expr[r] { BINOP($$, SHL, $l, $r, @$) }
         | expr[l] TSHR expr[r] { BINOP($$, SHR, $l, $r, @$) }
         | expr[l] TDOT2 expr[r] { BINOP($$, CONCAT, $l, $r, @$) }
         | expr[l] '+' expr[r] { BINOP($$, ADD, $l, $r, @$) }
         | expr[l] '-' expr[r] { BINOP($$, SUB, $l, $r, @$) }
         | expr[l] '*' expr[r] { BINOP($$, MUL, $l, $r, @$) }
         | expr[l] '/' expr[r] { BINOP($$, DIV, $l, $r, @$) }
         | expr[l] TSLASH2 expr[r] { BINOP($$, IDIV, $l, $r, @$) }
         | expr[l] '%' expr[r] { BINOP($$, MOD, $l, $r, @$) }
         | expr[l] '^' expr[r] { BINOP($$, POW, $l, $r, @$) }
         ;

unaryopcall: TNOT expr { UNARYOP($$, NOT, $expr, @$) }
           | '#' expr { UNARYOP($$, LEN, $expr, @$) }
           | '-' expr %prec NEG { UNARYOP($$, NEG, $expr, @$) }
           | '~' expr %prec BNOT { UNARYOP($$, BNOT, $expr, @$) }
           ;

globalvar: TGLOBAL namelist {
          $$ = new NGlobalVar(
              std::move(*$namelist),
              {});
          $$->location = @$;
          delete $namelist;
      }
      | TGLOBAL namelist '=' explist {
            $$ = new NGlobalVar(
                std::move(*$namelist),
                std::move(*$explist));
            $$->location = @$;
            delete $namelist;
            delete $explist;
      }
      ;

interface: TINTERFACE TIDENTIFIER[name] ':' type {
             $$ = new NInterface(std::move(*$name), ptr($type));
             $$->location = @$;
             delete $name;
         }
         | TINTERFACE TIDENTIFIER[name] '<' namelist '>' ':' type {
             $$ = new NInterface(std::move(*$name), ptr($type), std::move(*$namelist));
             $$->location = @$;
             delete $name;
             delete $namelist;
         }
         ;

localvar: TLOCAL namelist {
            $$ = new NLocalVar(
                std::move(*$namelist),
                {});
            $$->location = @$;
            delete $namelist;
        }
        | TLOCAL namelist '=' explist {
            $$ = new NLocalVar(
                std::move(*$namelist),
                std::move(*$explist));
            $$->location = @$;
            delete $namelist;
            delete $explist;
        }
        ;

localfunc: TLOCAL TFUNCTION TIDENTIFIER[name] funcparams funcret block TEND {
             $$ = new NLocalFunction(
                 {
                     {},
                     ptr($funcparams),
                     ptr($funcret),
                     ptr($block)
                 },
                 std::move(*$name));
             $$->location = @$;
             delete $name;
         }
         | TLOCAL TFUNCTION TIDENTIFIER[name] funcgenparams funcparams funcret block TEND {
             $$ = new NLocalFunction(
                 {
                    std::move(*$funcgenparams),
                    ptr($funcparams),
                    ptr($funcret),
                    ptr($block)
                 },
                 std::move(*$name));
             $$->location = @$;
             delete $name;
             delete $funcgenparams;
         }
         ;

function: TFUNCTION funcvar funcparams funcret block TEND {
            $$ = new NFunction(
                {
                    {},
                    ptr($funcparams),
                    ptr($funcret),
                    ptr($block)
                },
                ptr($funcvar));
            $$->location = @$;
        }
        | TFUNCTION funcvar funcgenparams funcparams funcret block TEND {
            $$ = new NFunction(
                {
                    std::move(*$funcgenparams),
                    ptr($funcparams),
                    ptr($funcret),
                    ptr($block)
                },
                ptr($funcvar));
            $$->location = @$;
            delete $funcgenparams;
        }
        | TFUNCTION funcvar ':' TIDENTIFIER[name] funcparams funcret block TEND {
            $$ = new NSelfFunction(
                {
                    {},
                    std::unique_ptr<NFuncParams>($funcparams),
                    std::unique_ptr<NType>($funcret),
                    std::unique_ptr<NBlock>($block)
                },
                std::move(*$name),
                std::unique_ptr<NExpr>($funcvar));
            $$->location = @$;
            delete $name;
        }
        | TFUNCTION funcvar ':' TIDENTIFIER[name] funcgenparams funcparams funcret block TEND {
            $$ = new NSelfFunction(
                {
                    std::move(*$funcgenparams),
                    std::unique_ptr<NFuncParams>($funcparams),
                    std::unique_ptr<NType>($funcret),
                    std::unique_ptr<NBlock>($block)
                },
                std::move(*$name),
                std::unique_ptr<NExpr>($funcvar));
            $$->location = @$;
            delete $funcgenparams;
            delete $name;
        }
        ;

funcret: %empty { $$ = nullptr; }
       | ':' rettype { $$ = $rettype; }
       ;

funcvar: TIDENTIFIER {
           $$ = new NIdent(std::move(*$1));
           $$->location = @$;
           delete $1;
       }
       | funcvar '.' TIDENTIFIER {
           $$ = new NTableAccess(
               std::unique_ptr<NExpr>($1),
               std::move(*$3));
           $$->location = @$;
           delete $3;
       }
       ;

funcgenparams: '<' '>' { $$ = new std::vector<NNameDecl>{}; }
             | '<' namelist '>' { $$ = $namelist; }
             ;

funcparams: '(' ')' { $$ = new NFuncParams(); $$->location = @$; }
          | '(' TDOT3 ')' {
              $$ = new NFuncParams({}, true);
              $$->location = @$;
          }
          | '(' namelist ')' {
              $$ = new NFuncParams(std::move(*$namelist), false);
              $$->location = @$;
              delete $namelist;
          }
          | '(' namelist ',' TDOT3 ')' {
              $$ = new NFuncParams(std::move(*$namelist), true);
              $$->location = @$;
              delete $namelist;
          }
          ;

forgeneric: TFOR namelist TIN explist TDO block TEND {
              $$ = new NForGeneric(
                  std::move(*$namelist),
                  std::move(*$explist),
                  std::unique_ptr<NBlock>($block));
              $$->location = @$;
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
              $$->location = @$;
              delete $2;
          }
          | TFOR TIDENTIFIER '=' expr ',' expr TDO block TEND {
              $$ = new NForNumeric(
                  std::move(*$2),
                  std::unique_ptr<NExpr>($4),
                  std::unique_ptr<NExpr>($6),
                  std::unique_ptr<NExpr>(nullptr),
                  std::unique_ptr<NBlock>($8));
              $$->location = @$;
              delete $2;
          }
          ;

if: TIF expr TTHEN block elseifseq else TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>($6));
      $$->location = @$;
      delete $5;
  }
  | TIF expr TTHEN block elseifseq TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          std::move(*$5),
          std::unique_ptr<NElse>(nullptr));
      $$->location = @$;
      delete $5;
  }
  | TIF expr TTHEN block else TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          {},
          std::unique_ptr<NElse>($5));
      $$->location = @$;
  }
  | TIF expr TTHEN block TEND {
      $$ = new NIf(
          std::unique_ptr<NExpr>($2),
          std::unique_ptr<NBlock>($4),
          {},
          std::unique_ptr<NElse>(nullptr));
      $$->location = @$;
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
          $$->location = @$;
      }
      ;

else: TELSE block { $$ = new NElse(std::unique_ptr<NBlock>($2)); $$->location = @$; }
    ;

repeat: TREPEAT block TUNTIL expr {
          $$ = new NRepeat(
              std::unique_ptr<NBlock>($2),
              std::unique_ptr<NExpr>($4));
          $$->location = @$;
      }
      ;

while: TWHILE expr TDO block TEND {
         $$ = new NWhile(
             std::unique_ptr<NExpr>($2),
             std::unique_ptr<NBlock>($4));
         $$->location = @$;
     }
     ;

goto: TGOTO TIDENTIFIER {
        $$ = new NGoto(std::move(*$2));
        delete $2;
    }
    ;

label: TCOLON2 TIDENTIFIER TCOLON2 {
         $$ = new NLabel(std::move(*$2));
         $$->location = @$;
         delete $2;
     }
     ;

assign: varlist '=' explist {
          $$ = new NAssignment(
              std::move(*$varlist),
              std::move(*$explist));
          $$->location = @$;
      }
      ;

functioncall: prefixexpr args {
                $$ = new NFunctionCall(ptr($prefixexpr), ptr($args));
                $$->location = @$;
            }
            | prefixexpr ':' TIDENTIFIER[name] args {
                $$ = new NFunctionSelfCall(
                    ptr($prefixexpr),
                    std::move(*$name),
                    ptr($args));
                $$->location = @$;
                delete $name;
            }
            ;

tableconstructor: '{' '}' { $$ = new NTableConstructor(); $$->location = @$; }
                | '{' fieldlist '}' {
                    $$ = new NTableConstructor(std::move(*$fieldlist));
                    $$->location = @$;
                    delete $fieldlist;
                }
                ;

fieldlist: field {
             $$ = new std::vector<std::unique_ptr<NField>>();
             $$->push_back(std::unique_ptr<NField>($field));
         }
         | fieldlist fieldsep field {
             $$ = $1;
             $$->push_back(std::unique_ptr<NField>($field));
         }
         | fieldlist fieldsep {
             $$ = $1;
         }
         ;

fieldsep: ',' | ';' ;

field: expr { $$ = new NFieldExpr(std::unique_ptr<NExpr>($expr)); $$->location = @$; }
     | TIDENTIFIER[name] '=' expr {
         $$ = new NFieldNamed(
             std::move(*$name),
             std::unique_ptr<NExpr>($expr));
         $$->location = @$;
         delete $name;
     }
     | '[' expr[key] ']' '=' expr[value] {
         $$ = new NFieldKey(
             std::unique_ptr<NExpr>($key),
             std::unique_ptr<NExpr>($value));
         $$->location = @$;
     }
     ;

functiondef: TFUNCTION funcparams funcret block TEND {
               $$ = new NFunctionDef(
                   std::unique_ptr<NFuncParams>($funcparams),
                   std::unique_ptr<NType>($funcret),
                   std::unique_ptr<NBlock>($block));
               $$->location = @$;
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

var: TIDENTIFIER {
       $$ = new NIdent(std::move(*$1));
       $$->location = @$;
       delete $1;
   }
   | prefixexpr '[' expr ']' {
       $$ = new NSubscript(
           std::unique_ptr<NExpr>($1),
           std::unique_ptr<NExpr>($3));
       $$->location = @$;
   }
   | prefixexpr '.' TIDENTIFIER {
       $$ = new NTableAccess(
           std::unique_ptr<NExpr>($1),
           std::move(*$3));
       $$->location = @$;
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
          $$->location = @$;
          $$->args.push_back(std::unique_ptr<NExpr>($1));
      }
      | argseq ',' expr {
          $$ = $1;
          $$->location = @$;
          $$->args.push_back(std::unique_ptr<NExpr>($3));
      }
      ;

args: '(' ')' { $$ = nullptr; }
    | '(' argseq ')' { $$ = $2; }
    ;

%%
