#include <cxxopts.hpp>
#include <string>

/* clang-format off */ // The factory pattern will all expand

struct SoodArgs {
  bool debug;
  bool no_verify;
  bool print_ast;
  bool print_llvm_ir;
  bool no_run;
  bool no_object_out;
  bool stop_after_ast;
  bool stop_after_llvm_ir;
  std::string input;
  std::string output;
  std::string ast_out;
  std::string llvm_ir_out;
  SoodArgs set_debug(bool b) { debug = b; return *this; }
  SoodArgs set_no_verify(bool b) { no_verify = b; return *this; }
  SoodArgs set_print_ast(bool b) { print_ast = b; return *this; }
  SoodArgs set_print_llvm_ir(bool b) { print_llvm_ir = b; return *this; }
  SoodArgs set_stop_after_ast(bool b) { stop_after_ast = b; return *this; }
  SoodArgs set_no_run(bool b) { no_run = b; return *this; }
  SoodArgs set_no_object_out(bool b) { no_object_out = b; return *this; }
  SoodArgs set_stop_after_llvm_ir(bool b) { stop_after_llvm_ir = b; return *this; }
  SoodArgs set_input(std::string s) { input = s; return *this; }
  SoodArgs set_output(std::string s) { output = s; return *this; }
  SoodArgs set_ast_out(std::string s) { ast_out = s; return *this; }
  SoodArgs set_llvm_ir_out(std::string s) { llvm_ir_out = s; return *this; }
};

/* clang-format on */

SoodArgs parse_args(int, char **);
