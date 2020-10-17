%{
#include <vector>
#include "ast.hpp"
extern int yylex();
void yyerror(const char *s) { std::printf("Error: %s\n", s); std::exit(1); }
NBlock *prg;
%}

%union {
  Node                 *node;
  NBlock               *n_block;
  NExpression          *n_expr;
  NStatement           *n_statement;
  NIdentifier          *n_identifier;
  NFunctionDeclaration *n_function_decl;
  NVariableDeclaration *n_variable_decl;
  NIfStatement         *n_if_stmt;

  std::string *string;
  int         val;

  std::vector<NExpression *>          *v_n_expr;
  std::vector<NVariableDeclaration *> *v_n_var_decl;
}

%token <string> /* types      */ TIDENT TFLOAT TINTEGER TSTRING
%token <string> /* grammar    */ TCOMMA TPERIOD TAND TCALLED TSEMIC TOFSTMTS
%token <string> /*            */ TFUNCTION TIS TOFDEFAULT TOFTYPE TOFVALUE
%token <string> /*            */ TRETURN TWITHARGS TWITH TAN TPARO TPARC
%token <string> /* constructs */ TIF TELSE TWHILE TUNTIL
%token <string> /*            */ TREAD TWRITE TTO TFROM
%token <val>    /* operators  */ TPLS TMNS TMUL TDIV TMOD
%token <val>    /* boolean    */ TEQ TNE TLT TLE TMT TME TNOT TNEG TALS TALT

%type <n_expr>    numeric string expr arithmetic
%type <n_expr>    binary_comparison unary_comparison func_call
%type <n_identifier>    identifier
%type <n_block>         program stmts block
%type <n_statement>     stmt var_decl func_decl if_stmt while_stmt until_stmt io_stmt
%type <n_variable_decl> func_decl_arg 
%type <v_n_var_decl>    func_decl_args
%type <v_n_expr>  func_call_args

%precedence TIF
%precedence TPARO

%left  TEQ TNE TLT TLE TMT TME TALS TALT
%left  TMOD
%left  TMUL TDIV
%left  TPLS TMNS

%right TELSE
%right TNOT TNEG

%start program

%%

program : stmts { prg = $1; }
        ;

stmts : stmt       { $$ = new NBlock(); $$->stmts.push_back($1); }
      | stmts stmt { $1->stmts.push_back($2); }
      ;

io_stmt : TREAD TFROM expr TTO expr TPERIOD { $$ = new NRead(*$3, *$5); }
        | TWRITE expr TTO expr TPERIOD { $$ = new NWrite(*$2, *$4); }
        ;

stmt : var_decl
     | func_decl
     | if_stmt
     | while_stmt
     | until_stmt
     | io_stmt
     | expr { $$ = new NExpressionStatement(*$1); }
     | identifier TIS expr TPERIOD  { $$ = new NAssignment(*$1, *$3); }
     | TRETURN expr TPERIOD { $$ = new NReturnStatement(*$2); }
     ;

func_call_args : TWITH expr
                 { $$ = new NExpressionList(); $$->push_back($2); }
               | func_call_args TCOMMA expr { $1->push_back($3); }
               | func_call_args TCOMMA TAND expr { $1->push_back($4); }
               ;

func_call : identifier TCALLED TPERIOD TPERIOD { $$ = new NFunctionCall(*$1); }
          | identifier TCALLED func_call_args TPERIOD TPERIOD
            { $$ = new NFunctionCall(*$1, *$3); delete $3; }
          ;

expr : identifier { $$ = $1; }
     | numeric
     | string
     | arithmetic
     | binary_comparison
     | unary_comparison
     | func_call
     | TPARO expr TPARC { $$ = $2; }
     ;

identifier : TIDENT { $$ = new NIdentifier(*$1); }
           ;

numeric : TINTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
        | TFLOAT   { $$ = new NFloat(atof($1->c_str())); delete $1; }
        ;

string : TSTRING { $$ = new NString(*$1); delete $1; }
       ;

arithmetic : expr TPLS expr
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | expr TMNS expr
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | expr TMUL expr
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | expr TDIV expr
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | expr TMOD expr
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           ;

binary_comparison : expr TEQ expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TNE expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TLT expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TLE expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TMT expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TALS expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  | expr TALT expr { $$ = new NBinaryExpression(*$1, $2, *$3); }
                  ;

unary_comparison : TNOT expr { $$ = new NUnaryExpression($1, *$2); }
                 | TNEG expr { $$ = new NUnaryExpression($1, *$2); }
                 ;

block : TCOMMA stmts TPERIOD TPERIOD { $$ = $2; } /* stmts creates a new block */
      | TCOMMA TPERIOD TPERIOD       { $$ = new NBlock(); }
      ;

var_decl : identifier TIS TAN identifier TOFVALUE expr TPERIOD
           { $$ = new NVariableDeclaration(*$4, *$1, $6); }
         | identifier TIS TAN identifier TPERIOD 
           { $$ = new NVariableDeclaration(*$1, *$4); }
         ;

func_decl_arg : TAN identifier identifier
                { $$ = new NVariableDeclaration(*$2, *$3); }
              | TAN identifier identifier TOFDEFAULT expr
                { $$ = new NVariableDeclaration(*$2, *$3, $5); }
              ;

func_decl_args : func_decl_arg { $$ = new NVariableList(); $$->push_back($1); }
               | func_decl_args TCOMMA func_decl_arg { $1->push_back($3); }
               | func_decl_args TCOMMA TAND func_decl_arg { $1->push_back($4); }
               ;

func_decl : identifier TIS TAN TFUNCTION TOFTYPE identifier TAND TOFSTMTS block
            { $$ = new NFunctionDeclaration(*$6, *$1, *$9); }
          | identifier TIS TAN TFUNCTION TOFTYPE identifier TWITHARGS
              func_decl_args TSEMIC TAND TOFSTMTS block
            { $$ = new NFunctionDeclaration(*$6, *$1, *$8, *$12); }
          ;

if_stmt : TIF expr block { $$ = new NIfStatement(*$2, *$3); }
        | if_stmt TELSE block { $<n_if_stmt>1->els = new NElseStatement(*$3); }
        | if_stmt TELSE if_stmt { $<n_if_stmt>1->els = $3; }
        ;

while_stmt : TWHILE expr block { $$ = new NWhileStatement(*$2, *$3); }
           ;

until_stmt : TUNTIL expr block { $$ = new NUntilStatement(*$2, *$3); }
           ;

%%
