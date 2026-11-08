#ifndef __CODE_GEN_HPP__
#define __CODE_GEN_HPP__

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

struct CodeGenException : public std::exception {
  std::string message = "Generic code generation exception";
  int line = 0; // Set by the innermost statement the exception passes through
  CodeGenException() {}
  CodeGenException(std::string message) : message(message) {}
  const char *what() const throw() { return message.c_str(); }
};

namespace llvm {
class TargetMachine;
}

class NBlock;
class NFunctionDeclaration;
class CodeGenContext;

extern llvm::LLVMContext LLVM_CTX;
extern llvm::IRBuilder<> BUILDER;

extern llvm::Type *BOOLEAN_TYPE;
extern llvm::Type *DOUBLE_TYPE;
extern llvm::Type *INTEGER_TYPE;
extern llvm::Type *STRING_TYPE;
extern llvm::Type *VOID_TYPE;

typedef std::tuple<llvm::Value *, llvm::Type *> ValTypeTuple;

/**
 * Name: CodeGenBlock
 * Construct: Class
 * Desc: A lexical scope, one is pushed for each function body and for each
 *   block of an `if`, `while` or `until`
 * Members:
 *   - locals: The variables declared directly within this scope, mapping the
 *     name to the pointer of the variable's storage and its type
 *   - fn_boundary: If true, name lookup stops at this scope (and continues at
 *     the global scope) so that a function cannot see its caller's locals
 */
class CodeGenBlock {
public:
  std::map<std::string, ValTypeTuple> locals;
  bool fn_boundary = false;
};

/**
 * Name: CodeGenContext
 * Construct: Class
 * Desc: Used to manage the AST -> LLVM IR -> Object phases of the compiler
 * Members:
 *   - scopes: Stack of lexical scopes (see `CodeGenBlock`), the bottom-most
 *     scope is the global scope; variables declared there are LLVM globals and
 *     so are visible from within functions
 *   - fn_main - Pointer to the "global" function of the Sood source code
 *   - module - The code is loaded to this object from the root node of the AST
 *   - fn_decls - The AST nodes of each declared function, used to fill in
 *     default arguments at call sites
 *   - string_constants - Cache of the global strings emitted so far, so equal
 *     literals share one global
 *   - target_machine - The native target, used for the module's data layout
 *     and to emit object code
 */
class CodeGenContext {
  std::vector<CodeGenBlock> scopes;
  std::unique_ptr<llvm::TargetMachine> target_machine;
  std::map<std::string, llvm::Constant *> string_constants;
  llvm::Function *fn_main = nullptr;

public:
  std::unique_ptr<llvm::Module> module;
  std::map<std::string, NFunctionDeclaration *> fn_decls;

  CodeGenContext(std::string module_name = "mod_main");
  ~CodeGenContext();

  void code_generate(NBlock &root);
  void print_llvm_ir();
  void print_llvm_ir_to_file(std::string &);
  bool verify_module();
  void optimize(unsigned);
  int code_run();
  int write_object(std::string &);

  /* ------ Scopes ------ */

  void push_scope(bool fn_boundary = false) {
    scopes.emplace_back();
    scopes.back().fn_boundary = fn_boundary;
  }
  void pop_scope() { scopes.pop_back(); }
  bool in_global_scope() { return scopes.size() == 1; }
  bool find_local(const std::string &, ValTypeTuple &);
  void set_local(const std::string &s, llvm::Value *val, llvm::Type *type) {
    scopes.back().locals[s] = std::make_tuple(val, type);
  }

  /* ------ Helpers for the AST nodes ------ */

  llvm::Function *current_function() {
    return BUILDER.GetInsertBlock()->getParent();
  }
  llvm::Function *get_libc(const std::string &);
  bool is_reserved(const std::string &);
  llvm::Constant *string_constant(const std::string &);
  llvm::Value *create_variable(const std::string &, llvm::Type *);
  void branch_to(llvm::BasicBlock *);
};

/* ------ Type helpers (see `src/ast-codegen.cpp`) ------ */

std::string type_name(llvm::Type *);
llvm::Constant *zero_value_for(CodeGenContext &, llvm::Type *);
llvm::Value *cast_to(llvm::Value *, llvm::Type *, const std::string &);

#endif
