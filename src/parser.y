%{
#include <vector>
#include "ast.hpp"
extern int yylex();
extern void yyerror(const char *);
NBlock *prg;
%}

%locations 

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

%token <string> /* types      */ TIDENT TFLOAT TINTEGER TSTRING TVOID
%token <string> /* grammar    */ TCOMMA TPERIOD TAND TCALLED TSEMIC TCOLON
%token <string> /*            */ TFUNCTION TIS TOFDEFAULT TOFTYPE TOFVALUE
%token <string> /*            */ TRETURN TWITHARGS TWITH TAN TPARO TPARC
%token <string> /*            */ TOFSTMT TOFSTMTS
%token <string> /* constructs */ TIF TELSE TWHILE TUNTIL TNOARGS TASARGS
%token <string> /*            */ TREAD TWRITE TTO TFROM
%token <val>    /* operators  */ TPLS TMNS TMUL TDIV TMOD
%token <val>    /* boolean    */ TEQ TNE TLT TLE TMT TME TNOT TNEG TALS TALT

%type <n_expr>          numeric string expr arithmetic
%type <n_expr>          binary_comparison unary_comparison func_call
%type <n_identifier>    identifier
%type <n_block>         program stmts block single_block
%type <n_statement>     stmt var_decl func_decl func_decl_single
%type <n_statement>     if_stmt while_stmt until_stmt io_stmt
%type <n_variable_decl> func_decl_arg
%type <v_n_var_decl>    func_decl_args
%type <v_n_expr>        func_call_args

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
     | func_decl_single
     | if_stmt
     | while_stmt
     | until_stmt
     | io_stmt
     | expr TPERIOD { $$ = new NExpressionStatement(*$1); }
     | identifier TIS expr TPERIOD  { $$ = new NAssignment(*$1, *$3); }
     | TRETURN expr TPERIOD { $$ = new NReturnStatement(*$2); }
     ;

func_call_args : TWITH expr
                 { $$ = new NExpressionList(); $$->push_back($2); }
               | func_call_args TCOMMA expr { $1->push_back($3); }
               | func_call_args TCOMMA TAND expr { $1->push_back($4); }
               ;

func_call : identifier TCALLED TWITH TNOARGS { $$ = new NFunctionCall(*$1); }
          | identifier TCALLED func_call_args TASARGS
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

var_decl : identifier TIS TAN identifier TOFVALUE expr TPERIOD
           { $$ = new NVariableDeclaration(*$4, *$1, $6); }
         | identifier TIS TAN identifier TPERIOD 
           { $$ = new NVariableDeclaration(*$4, *$1); }
         ;

single_block : stmt TPERIOD TPERIOD { $$ = new NBlock(); $$->stmts.push_back($1); }
             ;

block : stmts TPERIOD TPERIOD { $$ = $1; } /* stmts creates a new block */
      | TPERIOD TPERIOD       { $$ = new NBlock(); }
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

func_decl_single : identifier TIS TAN TFUNCTION TOFSTMT TCOLON single_block
                   {
                     NIdentifier *type = new NIdentifier("void");
                     $$ = new NFunctionDeclaration(*type, *$1, *$7);
                   }
                 | identifier TIS TAN TFUNCTION TWITHARGS TCOLON func_decl_args TSEMIC
                     TAND TOFSTMT TCOLON single_block
                   {
                     NIdentifier *type = new NIdentifier("void");
                     $$ = new NFunctionDeclaration(*type, *$1, *$7, *$12);
                   }
                 | identifier TIS TAN TFUNCTION TOFTYPE identifier TAND TOFSTMT TCOLON single_block
                   { $$ = new NFunctionDeclaration(*$6, *$1, *$10); }
                 | identifier TIS TAN TFUNCTION TOFTYPE identifier TWITHARGS TCOLON
                     func_decl_args TSEMIC TAND TOFSTMT TCOLON single_block
                   { $$ = new NFunctionDeclaration(*$6, *$1, *$9, *$14); }
                 ;

func_decl : identifier TIS TAN TFUNCTION TOFSTMTS TCOLON block
            {
              NIdentifier *type = new NIdentifier("void");
              $$ = new NFunctionDeclaration(*type, *$1, *$7);
            }
          | identifier TIS TAN TFUNCTION TWITHARGS TCOLON func_decl_args TSEMIC
              TAND TOFSTMTS TCOLON block
            {
              NIdentifier *type = new NIdentifier("void");
              $$ = new NFunctionDeclaration(*type, *$1, *$7, *$12);
            }
          | identifier TIS TAN TFUNCTION TOFTYPE identifier TAND TOFSTMTS TCOLON block
            { $$ = new NFunctionDeclaration(*$6, *$1, *$10); }
          | identifier TIS TAN TFUNCTION TOFTYPE identifier TWITHARGS TCOLON
              func_decl_args TSEMIC TAND TOFSTMTS TCOLON block
            { $$ = new NFunctionDeclaration(*$6, *$1, *$9, *$14); }
          ;

if_stmt : TIF expr TCOMMA block { $$ = new NIfStatement(*$2, *$4); }
        | if_stmt TELSE TCOMMA block { $<n_if_stmt>1->els = new NElseStatement(*$4); }
        | if_stmt TELSE if_stmt { $<n_if_stmt>1->els = $3; }
        ;

while_stmt : TWHILE expr TCOMMA block { $$ = new NWhileStatement(*$2, *$4); }
           ;

until_stmt : TUNTIL expr TCOMMA block { $$ = new NUntilStatement(*$2, *$4); }
           ;

%%
