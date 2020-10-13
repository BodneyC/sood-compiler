%{
#include <string>
#include "ast.hpp"
#include "parser.hpp"

#define SAVE_TOKEN (yylval.string = new std::string(yytext, yyleng));
#define TOKEN(t)   (yylval.val = t);

%}

%option noyywrap

%%

[ \t\n\r] ;
#.*       ;

"is equal to"              TOKEN(0); return TEQ;
"is not equal to"          TOKEN(1); return TNE;
"is less than"             TOKEN(2); return TLT;
"is less than or equal to" TOKEN(3); return TLE;
"is more than"             TOKEN(4); return TGT;
"is more than or equal to" TOKEN(5); return TGE;
"is not"                   TOKEN(6); return TNOT;
"is negative"              TOKEN(7); return TNEG;

"plus"                  TOKEN(7); return TPLS;
"minus"                 TOKEN(8); return TMNS;
"multiplied by"         TOKEN(9); return TMUL;
"divided by"            TOKEN(10); return TDIV;

","                     return TCOMMA;
"."                     return TPERIOD;
";"                     return TSEMIC;
"and"                   return TAND;
"called"                return TCALLED;
"function"              return TFUNCTION;
"is"                    return TIS;
"of default value"      return TOFDEFAULT;
"of type"               return TOFTYPE;
"of value"              return TOFVALUE;
"return"                return TRETURN;
"with arguments of:"    return TWITHARGS;
"with"                  return TWITH;
an?                     return TAN;
"("                     return TPARO;
")"                     return TPARC;

[a-zA-Z_][a-zA-Z0-9_]*  SAVE_TOKEN; return TIDENT;
[0-9]+.[0-9]+           SAVE_TOKEN; return TFLOAT;
[0-9]+                  SAVE_TOKEN; return TINTEGER;
["][^"]*["]             SAVE_TOKEN; return TSTRING;
['][^']*[']             SAVE_TOKEN; return TSTRING;

.                       printf("Invalid token\n"); yyterminate();

%%

extern int yylex();

/*
int main(void) {
  int token;
  while ((token = yylex()) != 0)
    printf("Token: %d (%s)\n", token, yytext);
  return 0;
}
*/
