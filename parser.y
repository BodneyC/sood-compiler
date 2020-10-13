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
  NExpression          *n_expression;
  NStatement           *n_statement;
  NIdentifier          *n_identifier;
  NFunctionDeclaration *n_function_decl;
  NVariableDeclaration *n_variable_decl;

  std::string *string;
  int         val;

  std::vector<NExpression *>          *v_n_expression;
  std::vector<NVariableDeclaration *> *v_n_var_decl;
}

%token <string> /* types     */ TIDENT TFLOAT TINTEGER TSTRING
%token <string> /* grammar   */ TCOMMA TPERIOD TAND TCALLED TSEMIC
%token <string> /*           */ TFUNCTION TIS TOFDEFAULT TOFTYPE TOFVALUE
%token <string> /*           */ TRETURN TWITHARGS TWITH TAN TPARO TPARC
%token <val>    /* operators */ TPLS TMNS TMUL TDIV
%token <val>    /* boolean   */ TEQ TNE TLT TLE TGT TGE TNOT TNEG

%type <val>             binary_comparator unary_comparator
%type <n_expression>    numeric string single expression
%type <n_identifier>    identifier
%type <n_block>         program stmts block
%type <n_statement>     stmt var_decl func_decl
%type <n_variable_decl> func_decl_arg 
%type <v_n_var_decl>    func_decl_args
%type <v_n_expression>  func_call_args

%right TNOT TNEG
%left  TPLS TMNS
%left  TMUL TDIV
%left  TEQ TNE TLT TLE TGT TGE
%precedence TPARO

%start program

%%

program: stmts { prg = $1; }
       ;

stmts : stmt       { $$ = new NBlock(); $$->stmts.push_back($1); }
      | stmts stmt { $1->stmts.push_back($2); }
      ;

stmt : var_decl
     | func_decl
     | expression         { $$ = new NExpressionStatement(*$1); }
     | TRETURN expression { $$ = new NReturn(*$2); }
     ;

expression : single
           | identifier TIS expression TPERIOD  { $$ = new NAssignment(*$1, *$3); }
           | identifier TCALLED TPERIOD { $$ = new NFunctionCall(*$1); }
           | identifier TCALLED func_call_args TPERIOD
             { $$ = new NFunctionCall(*$1, *$3); delete $3; }
           | single TPLS single
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | single TMNS single
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | single TMUL single
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | single TDIV single
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           | unary_comparator single
             { $$ = new NUnaryExpression($1, *$2); }
           | single binary_comparator single
             { $$ = new NBinaryExpression(*$1, $2, *$3); }
           ;

single: identifier { $$ = $1; }
      | numeric
      | string
       | TPARO single TPARC { $$ = $2; }
      ;

identifier: TIDENT { $$ = new NIdentifier(*$1); }
          ;

numeric : TINTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
        | TFLOAT   { $$ = new NFloat(atof($1->c_str())); delete $1; }
        ;

string : TSTRING { $$ = new NString(*$1); delete $1; }
       ;

block : TCOMMA stmts TPERIOD TPERIOD { $$ = $2; } /* stmts creates a new block */
      | TCOMMA TPERIOD TPERIOD       { $$ = new NBlock(); }
      ;

var_decl: identifier TIS TAN identifier TOFVALUE expression TPERIOD
          { $$ = new NVariableDeclaration(*$4, *$1, $6); }
        | identifier TIS TAN identifier TPERIOD 
          { $$ = new NVariableDeclaration(*$1, *$4); }
        ;

func_decl_arg: TAN identifier identifier
               { $$ = new NVariableDeclaration(*$2, *$3); }
             | TAN identifier identifier TOFDEFAULT expression
               { $$ = new NVariableDeclaration(*$2, *$3, $5); }
             ;

func_decl_args: func_decl_arg { $$ = new NVariableList(); $$->push_back($1); }
              | func_decl_args TCOMMA func_decl_arg { $1->push_back($3); }
              | func_decl_args TCOMMA TAND func_decl_arg { $1->push_back($4); }
              ;

func_decl: identifier TIS TAN TFUNCTION TOFTYPE identifier TAND TOFVALUE block
           { $$ = new NFunctionDeclaration(*$6, *$1, *$9); }
         | identifier TIS TAN TFUNCTION TOFTYPE identifier TWITHARGS
             func_decl_args TSEMIC TAND TOFVALUE block
           { $$ = new NFunctionDeclaration(*$6, *$1, *$8, *$12); }
         ;

func_call_args: TWITH expression
                { $$ = new NExpressionList(); $$->push_back($2); }
              | func_call_args TCOMMA expression { $1->push_back($3); }
              | func_call_args TCOMMA TAND expression { $1->push_back($4); }
              ;

binary_comparator: TEQ | TNE | TLT | TLE | TGT | TGE
                 ;

unary_comparator: TNOT | TNEG
                ;

%%
