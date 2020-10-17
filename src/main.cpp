#include <cxxopts.hpp>
#include <fstream>
#include <iostream>

#include "ast.hpp"
#include "codegen.hpp"

extern int yyparse();
extern FILE *yyin;
extern NBlock *prg;

/* clang-format off */ // The factory pattern will all expand, no thanks

struct SoodArgs {
  bool debug;
  bool print_ast;
  bool stop_after_ast;
  std::string input;
  std::string output;
  std::string ast_out;
  SoodArgs set_debug(bool b) { debug = b; return *this; }
  SoodArgs set_print_ast(bool b) { print_ast = b; return *this; }
  SoodArgs set_stop_after_ast(bool b) { stop_after_ast = b; return *this; }
  SoodArgs set_input(std::string s) { input = s; return *this; }
  SoodArgs set_output(std::string s) { output = s; return *this; }
  SoodArgs set_ast_out(std::string s) { ast_out = s; return *this; }
};

SoodArgs parse_args(int argc, char **argv) {
  cxxopts::Options opts("sood", "Compiler for the Sood programming language");
  opts.add_options()
    ("d,debug",           "Enable debugging")
    ("print-ast",         "Print generated AST")
    ("S,stop-after-ast",  "Stop after generating the AST")
    ("write-ast-to-file", "Write the AST to the specified file",
     cxxopts::value<std::string>()->default_value("a.sood.ast"))
    ("i,input",           "Input file name, else stdin",
     cxxopts::value<std::string>())
    ("o,output",          "Output file name",
     cxxopts::value<std::string>()->default_value("a.sood.out"))
    ("positional",        "Arguments entered without an option",
     cxxopts::value<std::vector<std::string>>())
    ;
  opts.parse_positional({"input", "output", "positional"});
  auto res = opts.parse(argc, argv);
  return SoodArgs()
    .set_debug(res["debug"].as<bool>())
    .set_print_ast(res["print-ast"].as<bool>())
    .set_input(res.count("input") ? res["input"].as<std::string>() : "")
    .set_output(res.count("output") ? res["output"].as<std::string>() : "")
    .set_stop_after_ast(res["stop-after-ast"].as<bool>())
    .set_ast_out(res.count("write-ast-to-file") ?
        res["write-ast-to-file"].as<std::string>() : "");
}

/* clang-format on */

int main(int argc, char **argv) {

  SoodArgs args = parse_args(argc, argv);

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

  // Input options
  if (args.input != "")
    yyin = std::fopen(args.input.c_str(), "r");
  yyparse();
  if (args.input != "")
    std::fclose(yyin);

  // AST outputs
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
  create_core_fns(ctx);
  ctx.code_generate(*prg);
  ctx.code_run();

  return 0;
}
