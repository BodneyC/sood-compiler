%{
#include <string>
#include "ast.hpp"
#include "parser.hpp"

#define SAVE_TOKEN (yylval.string = new std::string(yytext, yyleng));
#define TOKEN(t)   (yylval.val = t);

void yyerror(const char *s) {
  std::printf("Lexer/parser error on line %d: %s\n", yylineno, s);
  std::exit(1);
}

%}

%option noyywrap
%option yylineno

%%

[ \t\n\r]                  ;
#.*                        ;

"is equal to"              TOKEN(OP_EQUAL_TO);           return TEQ;
"is not equal to"          TOKEN(OP_NOT_EQUAL_TO);       return TNE;
"is less than"             TOKEN(OP_LESS_THAN);          return TLT;
"is less than or equal to" TOKEN(OP_LESS_THAN_EQUAL_TO); return TLE;
"is more than"             TOKEN(OP_MORE_THAN);          return TMT;
"is more than or equal to" TOKEN(OP_MORE_THAN_EQUAL_TO); return TME;
"not"                      TOKEN(OP_NOT);                return TNOT;
"negative"                 TOKEN(OP_NEGATIVE);           return TNEG;
"also"                     TOKEN(OP_ALSO);               return TALS;
"alternatively"            TOKEN(OP_ALTERNATIVELY);      return TALT;

"plus"                     TOKEN(OP_PLUS);               return TPLS;
"minus"                    TOKEN(OP_MINUS);              return TMNS;
"multiplied by"            TOKEN(OP_MULTIPLIED_BY);      return TMUL;
"divided by"               TOKEN(OP_DIVIDED_BY);         return TDIV;
"modulo"                   TOKEN(OP_MODULO);             return TMOD;

","                        return TCOMMA;
"."                        return TPERIOD;
";"                        return TSEMIC;
":"                        return TCOLON;
"and"                      return TAND;
"called"                   return TCALLED;
"function"                 return TFUNCTION;
"is"                       return TIS;
"of default value"         return TOFDEFAULT;
"of type"                  return TOFTYPE;
"of value"                 return TOFVALUE;
"of statements"            return TOFSTMTS;
"of the statement"         return TOFSTMT;
"no arguments"             return TNOARGS;
"as an argument"           return TASARGS;
"as arguments"             return TASARGS;
"return"                   return TRETURN;
"with arguments of"        return TWITHARGS;
"with"                     return TWITH;
an?                        return TAN;
"("                        return TPARO;
")"                        return TPARC;

"if"                       return TIF;
"else"                     return TELSE;
"while"                    return TWHILE;
"until"                    return TUNTIL;

"read"                     return TREAD;
"write"                    return TWRITE;
"to"                       return TTO;
"from"                     return TFROM;

[a-zA-Z_][a-zA-Z0-9_]*     SAVE_TOKEN; return TIDENT;
[0-9]+\.[0-9]+             SAVE_TOKEN; return TFLOAT;
[0-9]+                     SAVE_TOKEN; return TINTEGER;
["][^"]*["]                SAVE_TOKEN; return TSTRING;
['][^']*[']                SAVE_TOKEN; return TSTRING;

.                          printf("Invalid token\n"); yyterminate();

%%

/*
extern int yylex();

int main(void) {
  int token;
  while ((token = yylex()) != 0)
    printf("Token: %d (%s)\n", token, yytext);
  return 0;
}
*/
