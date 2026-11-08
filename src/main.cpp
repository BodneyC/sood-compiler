#include <cstdio>
#include <fstream>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <spdlog/cfg/env.h>
#include <spdlog/spdlog.h>

#include "ast.hpp"
#include "cli.hpp"
#include "codegen.hpp"

extern int yyparse();
extern FILE *yyin;
extern NBlock *prg;
extern char **environ;

/**
 * Name: run_command
 * Construct: Function
 * Desc: Runs a command (searching the `PATH`) and waits for it, returning its
 *   exit status, or -1 if it couldn't be started. The command's output is not
 *   captured
 * Args:
 *   - argv: The command and its arguments
 */
static int run_command(std::vector<std::string> argv) {
  std::vector<char *> c_argv;
  for (auto &arg : argv)
    c_argv.push_back(&arg[0]);
  c_argv.push_back(nullptr);
  pid_t pid;
  if (posix_spawnp(&pid, c_argv[0], nullptr, nullptr, c_argv.data(), environ))
    return -1;
  int status;
  if (waitpid(pid, &status, 0) == -1)
    return -1;
  return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/** Maximum length of back-trace to be displayed by SPDLog */
const int BT_VOL = 32;

int main(int argc, char **argv) {
  spdlog::enable_backtrace(BT_VOL);
  spdlog::cfg::load_env_levels();
  spdlog::info("Starting Sood compiler...");

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
  if (yyparse() || !prg)
    return 1;

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
  try {
    ctx.code_generate(*prg);
  } catch (CodeGenException &e) {
    if (e.line)
      spdlog::error("Error near line {}: {}", e.line, e.what());
    else
      spdlog::error("Error: {}", e.what());
    return 1;
  }

  if (!args.no_verify) {
    spdlog::info("Verifying LLVM module");
    if (!ctx.verify_module()) {
      spdlog::error("LLVM module verification failed");
      return 1;
    }
  }

  if (args.opt_level) {
    spdlog::info("Optimizing at level {}", args.opt_level);
    ctx.optimize(args.opt_level);
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

  /**
   * Run the code (the LLVM module's main function) from within the compiler,
   *   exiting with its status
   */
  if (args.run_llvm_ir) {
    spdlog::info("Running LLVM module...");
    try {
      return ctx.code_run();
    } catch (CodeGenException &e) {
      spdlog::error("{}", e.what());
      return 1;
    }
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
    std::vector<char> obj_fname_c(obj_fname.begin(), obj_fname.end());
    obj_fname_c.push_back('\0');
    int fd = mkstemp(obj_fname_c.data());
    if (fd == -1) {
      spdlog::error("Could not open temporary file");
      return 1;
    }
    close(fd);
    obj_fname = std::string(obj_fname_c.data());
  }

  spdlog::debug("Writing object code to {}", obj_fname);
  if (ctx.write_object(obj_fname))
    return 1;

  if (args.stop_after_object) {
    spdlog::info("Object code written to {}", obj_fname);
    return 0;
  }

  /**
   * Sub-process to GCC (or LD) to link the object with the C runtime
   *   libraries and libc
   */
  int status = run_command({"gcc", "-o", args.output, obj_fname});
  std::remove(obj_fname.c_str());
  if (status) {
    spdlog::error("GCC compilation failed");
    return 1;
  }
  spdlog::info("Native binary written to {}", args.output);

  spdlog::info("Finishing Sood compiler");
  return 0;
}
