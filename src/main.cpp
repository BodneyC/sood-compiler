#include <cxxopts.hpp>
#include <fstream>
#include <iostream>

#include "ast.hpp"
#include "codegen.hpp"

extern int yyparse();
extern FILE *yyin;
extern NBlock *prg;

/* clang-format off */ // The factory pattern will all expand, and the CXXOpts 
                       //   will get all upset

struct SoodArgs {
  bool debug;
  bool print_ast;
  bool print_llvm_ir;
  bool stop_after_ast;
  bool stop_after_llvm_ir;
  std::string input;
  std::string output;
  std::string ast_out;
  std::string llvm_ir_out;
  SoodArgs set_debug(bool b) { debug = b; return *this; }
  SoodArgs set_print_ast(bool b) { print_ast = b; return *this; }
  SoodArgs set_print_llvm_ir(bool b) { print_llvm_ir = b; return *this; }
  SoodArgs set_stop_after_ast(bool b) { stop_after_ast = b; return *this; }
  SoodArgs set_stop_after_llvm_ir(bool b) { stop_after_llvm_ir = b; return *this; }
  SoodArgs set_input(std::string s) { input = s; return *this; }
  SoodArgs set_output(std::string s) { output = s; return *this; }
  SoodArgs set_ast_out(std::string s) { ast_out = s; return *this; }
  SoodArgs set_llvm_ir_out(std::string s) { llvm_ir_out = s; return *this; }
};

SoodArgs parse_args(int argc, char **argv) {
  cxxopts::Options opts("sood", "Compiler for the Sood programming language");
  opts.add_options()
    ("h,help",                  "Show this help message")
    ("d,debug",                 "Enable debugging")
    ("a,print-ast",             "Print generated AST to stdout")
    ("l,print-llvm-ir",         "Print generated LLVM IR to stdout")
    ("S,stop-after-ast",        "Stop after generating the AST")
    ("C,stop-after-llvm-ir",    "Stop after generating the LLVM IR")
    ("A,write-ast-to-file",     "Write the AST to `<input>.ast`")
    ("L,write-llvm-ir-to-file", "Write the LLVM IR to `<input>.ll`")
    ("i,input",                 "Sood source file, else stdin",
     cxxopts::value<std::string>())
    ("o,output",          "Output file name",
     cxxopts::value<std::string>()->default_value("a.sood.out"))
    ("positional",        "Arguments entered without an option",
     cxxopts::value<std::vector<std::string>>());
  opts.parse_positional({"input", "output", "positional"});
  auto res = opts.parse(argc, argv);
  if (res.count("help")) {
    std::cout << opts.help() << std::endl;
    exit(0);
  }
  return SoodArgs()
    .set_debug(res["debug"].as<bool>())
    .set_print_ast(res["print-ast"].as<bool>())
    .set_print_llvm_ir(res["print-llvm-ir"].as<bool>())
    .set_input(res.count("input") ? res["input"].as<std::string>() : "")
    .set_output(res.count("output") ? res["output"].as<std::string>() : "")
    .set_stop_after_ast(res["stop-after-ast"].as<bool>())
    .set_stop_after_llvm_ir(res["stop-after-llvm-ir"].as<bool>())
    .set_ast_out(res.count("write-ast-to-file") ?
      res.count("input") ?
        res["input"].as<std::string>() + ".ast" :
        "a.sood.ast" :
      ""
    )
    .set_llvm_ir_out(res.count("write-llvm-ir-to-file") ?
      res.count("input") ?
        res["input"].as<std::string>() + ".ll" :
        "a.sood.ll" :
      ""
    );
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
  // create_core_fns(ctx);
  ctx.code_generate(*prg);
  ctx.verify_module();

  if (args.print_llvm_ir)
    ctx.print_llvm_ir();
  if (args.llvm_ir_out != "")
    ctx.print_llvm_ir_to_file(args.llvm_ir_out);

  if (args.stop_after_llvm_ir)
    return 0;

  // ctx.code_run();

  return 0;
}
