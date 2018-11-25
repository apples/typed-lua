%{
#include <string>
#include "node.hpp"
#include "parser.hpp"
#define SAVE_TOKEN yylval.string = new std::string(yytext, yyleng)
#define TOKEN(t) (yylval.token = t)
extern "C" int yywrap() { }
%}

%%

[ \t\n]                             ;
[a-zA-Z_][a-zA-Z0-9_]*              SAVE_TOKEN; return TIDENTIFIER;
[0-9]+(\.[0-9]+([eE]-?[0-9]+)?)?    SAVE_TOKEN; return TNUMBER;
\"(\\.|[^\\"])*\"|'(\\.|[^\\'])*'   SAVE_TOKEN; return TSTRING;
"="                                 return TOKEN(TEQUAL);
"=="                                return TOKEN(TCEQ);
"!="                                return TOKEN(TCNE);
"<"                                 return TOKEN(TCLT);
">"                                 return TOKEN(TCGT);
"<="                                return TOKEN(TCLE);
">="                                return TOKEN(TCGE);
"("                                 return TOKEN(TLPAREN);
")"                                 return TOKEN(TRPAREN);
"{"                                 return TOKEN(TLBRACE);
"}"                                 return TOKEN(TRBRACE);
"["                                 return TOKEN(TLSQBR);
"]"                                 return TOKEN(TRSQBR);
"."                                 return TOKEN(TDOT);
","                                 return TOKEN(TCOMMA);
":"                                 return TOKEN(TCOLON);
";"                                 return TOKEN(TSEMICOLON);
"+"                                 return TOKEN(TPLUS);
"-"                                 return TOKEN(TMINUS);
"*"                                 return TOKEN(TSTAR);
"/"                                 return TOKEN(TSLASH);
"//"                                return TOKEN(TSLASH2);
".."                                return TOKEN(TDOT2);
"..."                               return TOKEN(TDOT3);
"::"                                return TOKEN(TCOLON2);
.                                   printf("Unknown token!\n"); yyterminate();

%%
