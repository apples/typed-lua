%option prefix="typedlua"
%option reentrant bison-bridge bison-locations
%option noyywrap nounput noinput
%option yylineno

%top{
#include <string>
#include "node.hpp"
#include "parser.hpp"
#define YYSTYPE TYPEDLUASTYPE
#define YYLTYPE TYPEDLUALTYPE
}

%{
#define SAVE_TOKEN yylval->string = new std::string(yytext, yyleng)
#define YY_USER_ACTION yylloc->first_line = yylloc->last_line = yylineno;
%}

%%

[ \t\n]                             ;
--.*\n                              ;
"="                                 |
"<"                                 |
">"                                 |
"("                                 |
")"                                 |
"{"                                 |
"}"                                 |
"["                                 |
"]"                                 |
"."                                 |
","                                 |
":"                                 |
";"                                 |
"|"                                 |
"&"                                 |
"~"                                 |
"%"                                 |
"#"                                 |
"^"                                 |
"+"                                 |
"-"                                 |
"*"                                 |
"/"                                 return yytext[0];
"=="                                return TCEQ;
"~="                                return TCNE;
"<="                                return TCLE;
">="                                return TCGE;
"<<"                                return TSHL;
">>"                                return TSHR;
"//"                                return TSLASH2;
".."                                return TDOT2;
"..."                               return TDOT3;
"::"                                return TCOLON2;
"and"                               return TAND;
"break"                             return TBREAK;
"do"                                return TDO;
"else"                              return TELSE;
"elseif"                            return TELSEIF;
"end"                               return TEND;
"false"                             return TFALSE;
"for"                               return TFOR;
"function"                          return TFUNCTION;
"goto"                              return TGOTO;
"if"                                return TIF;
"in"                                return TIN;
"local"                             return TLOCAL;
"nil"                               return TNIL;
"not"                               return TNOT;
"or"                                return TOR;
"repeat"                            return TREPEAT;
"return"                            return TRETURN;
"then"                              return TTHEN;
"true"                              return TTRUE;
"until"                             return TUNTIL;
"while"                             return TWHILE;
[a-zA-Z_][a-zA-Z0-9_]*              SAVE_TOKEN; return TIDENTIFIER;
[0-9]+(\.[0-9]+([eE]-?[0-9]+)?)?    SAVE_TOKEN; return TNUMBER;
\"(\\.|[^\\"])*\"|'(\\.|[^\\'])*'   SAVE_TOKEN; return TSTRING;
.                                   printf("Unknown token!\n"); yyterminate();

%%
