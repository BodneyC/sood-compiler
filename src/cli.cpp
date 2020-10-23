#include "cli.hpp"

/* clang-format off */ // The factory pattern will all expand

SoodArgs parse_args(int argc, char **argv) {
  cxxopts::Options opts("sood", "Compiler for the Sood programming language");
  opts.add_options()
    ("h,help",               "Show this help message")
    ("d,debug",              "Enable debugging")
    ("a,print-ast",          "Print generated AST to stdout")
    ("l,print-llvm-ir",      "Print generated LLVM IR to stdout")
    ("V,no-verify",          "Disable LLVM verification")
    ("R,run-llvm-ir",        "Run module within the compiler")
    ("S,stop-after-ast",     "Stop after generating the AST")
    ("C,stop-after-llvm-ir", "Stop after generating the LLVM IR")
    ("O,stop-after-object",  "Stop after writing object file")
    ("i,input",              "Sood source file, else stdin", cxxopts::value<std::string>())
    ("o,output",             "Output file name",
     cxxopts::value<std::string>()->default_value(DEFAULT_OUT));
  opts.parse_positional({"input"});
  auto res = opts.parse(argc, argv);
  if (res.count("help")) {
    std::cout << opts.help() << std::endl;
    exit(0);
  }
  std::string output = res["output"].as<std::string>();
  if(res.count("input") && output == DEFAULT_OUT) {
    std::string input = res["input"].as<std::string>();
    if (res["stop-after-ast"].as<bool>())
      output = input + ".ast";
    else if (res["stop-after-llvm-ir"].as<bool>())
      output = input + ".ll";
    else if (res["stop-after-object"].as<bool>())
      output = input + ".o";
  }
  return SoodArgs()
    .set_debug(res["debug"].as<bool>())
    .set_print_ast(res["print-ast"].as<bool>())
    .set_print_llvm_ir(res["print-llvm-ir"].as<bool>())
    .set_no_verify(res["no-verify"].as<bool>())
    .set_run_llvm_ir(res["run-llvm-ir"].as<bool>())
    .set_stop_after_ast(res["stop-after-ast"].as<bool>())
    .set_stop_after_llvm_ir(res["stop-after-llvm-ir"].as<bool>())
    .set_stop_after_object(res["stop-after-object"].as<bool>())
    .set_input(res.count("input") ? res["input"].as<std::string>() : "")
    .set_output(output);
}

/* clang-format on */
