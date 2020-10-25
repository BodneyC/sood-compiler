#include <fstream>
#include <iostream>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "ast.hpp"
#include "cli.hpp"
#include "codegen.hpp"
#include "subprocess.hpp"

extern int yyparse();
extern FILE *yyin;
extern NBlock *prg;

/** Maximum length of back-trace to be displayed by SPDLog */
const int BT_VOL = 32;

int main(int argc, char **argv) {
  spdlog::info("Starting Sood compiler...");
  spdlog::enable_backtrace(BT_VOL);
  spdlog::cfg::load_env_levels();

  SoodArgs args = parse_args(argc, argv);

  /**
   * If an input file is specified on the command line, use that as the source
   *   of Sood code
   */
  if (args.input != "") {
    spdlog::debug("Reading input from file {}", args.input);
    /**
     * The rest of this block verifies that the input file both exists and is
     *   not empty
     */
    if (!(yyin = std::fopen(args.input.c_str(), "r"))) {
      spdlog::error("Input file {} does not exist, exiting...", args.input);
      std::exit(1);
    }
    fseek(yyin, 0, SEEK_END);
    if (!ftell(yyin))
      spdlog::warn("Input file {} is empty", args.input);
    fseek(yyin, 0, SEEK_SET);
  }

  /** Parse the source code using the generated parser from Bison */
  yyparse();

  /**
   * If an input file was specified, we are now finished with it and can
   *   close it
   */
  if (args.input != "")
    std::fclose(yyin);

  if (args.print_ast) {
    spdlog::debug("Printing AST to stdout...");
    std::cout << *prg << std::endl;
  }

  /**
   * If the option has been given to stop after generating the AST, the
   *   string representation of the AST is written to the output CLI option
   */
  if (args.stop_after_ast) {
    spdlog::info("Writing AST to {}...", args.output);
    std::ofstream ast_out(args.output);
    ast_out << *prg << std::endl;
    ast_out.close();
    spdlog::info("Stopping after AST generation");
    return 0;
  }

  CodeGenContext ctx;
  ctx.code_generate(*prg);

  if (!args.no_verify) {
    spdlog::info("Verifying LLVM module");
    ctx.verify_module();
  }

  if (args.print_llvm_ir) {
    spdlog::debug("Printing LLVM IR to stdout...");
    ctx.print_llvm_ir();
  }

  /**
   * If the option has been given to stop after generating the LLVM IR, the
   *   IR code is written to the output CLI option
   */
  if (args.stop_after_llvm_ir) {
    spdlog::info("Writing LLVM IR to {}...", args.output);
    ctx.print_llvm_ir_to_file(args.output);
    spdlog::info("Stopping after LLVM IR generation");
    return 0;
  }

  /** Run the code (the LLVM module's main function) from within the compiler */
  if (args.run_llvm_ir) {
    spdlog::info("Running LLVM module...");
    ctx.code_run();
  }

  std::string obj_fname = args.output;

  /**
   * If the option has been given to compile the code to an executable, then we
   *   could but shouldn't use the output CLI option (filename) for the object
   *   code as well as the name of the executable, so, we generate a temporary
   *   file containing the object code for use in the GCC or LD sub-process to
   *   create the resulting binary
   */
  if (!args.stop_after_object) {
    auto pos = obj_fname.rfind("/");
    if (pos != std::string::npos)
      obj_fname.erase(0, pos + 1);
    obj_fname = "/tmp/" + obj_fname + ".o.XXXXXX";
    char *obj_fname_c = strdup(obj_fname.c_str());
    int fd = mkstemp(obj_fname_c);
    if (fd == -1) {
      spdlog::error("Could not open temporary file");
      std::exit(1);
    }
    obj_fname = std::string(obj_fname_c);
  }

  spdlog::debug("Writing object code to {}", obj_fname);
  ctx.write_object(obj_fname);

  if (args.stop_after_object)
    return 0;

  /**
   * Sub-process to GCC (or LD) to link the object with the C runtime
   *   libraries and, optioinally, libc
   */
  subprocess::popen gcc_cmd("gcc",
                            {"-o", args.output.c_str(), obj_fname.c_str()});
  /*
   * Note: This also works but I may as well just use GCC
   *   ld --verbose -L/usr/lib -lc \
   *     -dynamic-linker \
   *     /lib64/ld-linux-x86-64.so.2 \
   *     /usr/lib/Scrt1.o \
   *     /usr/lib/crti.o \
   *     /usr/lib/gcc/x86_64-pc-linux-gnu/10.2.0/crtbeginS.o \
   *     /usr/lib/gcc/x86_64-pc-linux-gnu/10.2.0/crtendS.o  \
   *     <object file> \
   *     -o <binary> \
   *     /usr/lib/crtn.o
   */
  if (gcc_cmd.wait()) {
    spdlog::error("GCC compilation failed:");
    std::cerr << gcc_cmd.stderr().rdbuf() << std::endl;
  } else {
    spdlog::info("Native binary written to {}", args.output);
  }

  spdlog::info("Finishing Sood compiler");
  return 0;
}
