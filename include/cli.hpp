#include <cxxopts.hpp>
#include <string>

/* clang-format off */ // The factory pattern will all expand

const std::string DEFAULT_OUT = "a.sood.out";

struct SoodArgs {
  bool debug;
  bool no_verify;
  bool print_ast;
  bool print_llvm_ir;
  bool run_llvm_ir;
  bool stop_after_ast;
  bool stop_after_llvm_ir;
  bool stop_after_object;
  std::string input;
  std::string output;
  SoodArgs set_debug(bool b) { debug = b; return *this; }
  SoodArgs set_no_verify(bool b) { no_verify = b; return *this; }
  SoodArgs set_print_ast(bool b) { print_ast = b; return *this; }
  SoodArgs set_print_llvm_ir(bool b) { print_llvm_ir = b; return *this; }
  SoodArgs set_stop_after_ast(bool b) { stop_after_ast = b; return *this; }
  SoodArgs set_run_llvm_ir(bool b) { run_llvm_ir = b; return *this; }
  SoodArgs set_stop_after_object(bool b) { stop_after_object = b; return *this; }
  SoodArgs set_stop_after_llvm_ir(bool b) { stop_after_llvm_ir = b; return *this; }
  SoodArgs set_input(std::string s) { input = s; return *this; }
  SoodArgs set_output(std::string s) { output = s; return *this; }
};

/* clang-format on */

SoodArgs parse_args(int, char **);
