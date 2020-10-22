#include <fstream>
#include <iostream>

#include "ast.hpp"
#include "codegen.hpp"
#include "cli.hpp"

extern int yyparse();
extern FILE *yyin;
extern NBlock *prg;

int main(int argc, char **argv) {

  SoodArgs args = parse_args(argc, argv);

  if (args.input != "")
    yyin = std::fopen(args.input.c_str(), "r");
  yyparse();
  if (args.input != "")
    std::fclose(yyin);

  if (args.print_ast)
    std::cout << *prg << std::endl;
  if (args.ast_out != "") {
    std::ofstream ast_out(args.ast_out);
    ast_out << *prg << std::endl;
    ast_out.close();
  }

  if (args.stop_after_ast)
    return 0;

  CodeGenContext ctx;
  ctx.code_generate(*prg);

  if (!args.no_verify)
    ctx.verify_module();

  if (args.print_llvm_ir)
    ctx.print_llvm_ir();

  if (args.llvm_ir_out != "")
    ctx.print_llvm_ir_to_file(args.llvm_ir_out);

  if (args.stop_after_llvm_ir)
    return 0;

  if(!args.no_run)
    ctx.code_run();

  if(!args.no_object_out && args.output != "")
    ctx.write_object(args.output);

  return 0;
}
