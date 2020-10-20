#include "cli.hpp"

/* clang-format off */ // The factory pattern will all expand

SoodArgs parse_args(int argc, char **argv) {
  cxxopts::Options opts("sood", "Compiler for the Sood programming language");
  opts.add_options()
    ("h,help",                  "Show this help message")
    ("d,debug",                 "Enable debugging")
    ("V,no-verify",             "Disable LLVM verification")
    ("a,print-ast",             "Print generated AST to stdout")
    ("l,print-llvm-ir",         "Print generated LLVM IR to stdout")
    ("R,no-run",                "Don't run compiled code")
    ("O,no-object-out",         "Don't write object file")
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
    .set_no_verify(res["no-verify"].as<bool>())
    .set_print_ast(res["print-ast"].as<bool>())
    .set_print_llvm_ir(res["print-llvm-ir"].as<bool>())
    .set_no_run(res["no-run"].as<bool>())
    .set_no_object_out(res["no-object-out"].as<bool>())
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
