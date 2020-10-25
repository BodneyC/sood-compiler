#ifndef __CODE_GEN_HPP__
#define __CODE_GEN_HPP__

#include <iostream>
#include <stack>
#include <typeinfo>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/TargetSelect.h>
#include "llvm/Support/TargetRegistry.h"
#include <llvm/Support/raw_ostream.h>

struct CodeGenException : public std::exception {
  std::string message = "Generic code generation exception";
  CodeGenException() {}
  CodeGenException(std::string message) : message(message) {}
  const char *what() const throw() { return message.c_str(); }
};

struct SoodStringException : public std::exception {
  std::string message = "Generic sood string exception";
  SoodStringException() {}
  SoodStringException(std::string message) : message(message) {}
  const char *what() const throw() { return message.c_str(); }
};

class NBlock;
class CodeGenContext;

extern llvm::LLVMContext LLVM_CTX;
extern llvm::IRBuilder<> BUILDER;

extern llvm::Type *DOUBLE_TYPE;
extern llvm::Type *INTEGER_TYPE;
extern llvm::Type *STRING_TYPE;

typedef std::tuple<llvm::Value *, llvm::Type *> ValTypeTuple;

llvm::Constant *get_i8_str_ptr(char const *, llvm::Twine const &);

/**
 * Name: CodeGenBlock
 * Construct: Class
 * Desc: Context information for any given block
 * Members:
 *   - block: A pointer to the LLVM block corresponding to the CodeGenBlock
 *     instance
 *   - ret_val: A pointer to the return value of the block
 *   - locals: A list of the in-scope variables for the block
 * Notes:
 *   - The "global" function's locals are not currently available to
 *     child-blocks, this is yet to be implemented
 *   - `ret_val` is not currently used. But, as LLVM doesn't seem to like
 *     multiple return statements in a function, this may be used at one point
 *     and a single `return` enforced
 */
class CodeGenBlock {
public:
  llvm::BasicBlock *block;
  llvm::Value *ret_val;
  std::map<std::string, ValTypeTuple> locals;
};

/**
 * Name: CodeGenContext
 * Construct: Class
 * Desc: Used to manage the AST -> LLVM IR -> Object phases of the compiler
 * Members:
 *   - blocks - Stack of CodeGenBlock* referring to the block in the Sood
 *     source code
 *   - fn_main - Pointer to the "global" functino of the Sood source code
 *   - module - The code is loaded to this object from the root node of the AST
 *   - printf_function - Creation of the `printf` function in the resulting IR,
 *     linked to libc after code generation
 *   - fmt_specifiers - Global string references for `"%s"` and `"%d"`
 */
class CodeGenContext {
  std::stack<CodeGenBlock *> blocks;
  llvm::Function *fn_main;
  llvm::Function *create_fn_printf();

public:
  llvm::Module *module;
  llvm::Function *printf_function;
  std::map<std::string, llvm::Value *> fmt_specifiers;

  CodeGenContext(std::string module_name = "mod_main");

  void code_generate(NBlock &root);
  void print_llvm_ir();
  void print_llvm_ir_to_file(std::string &);
  void verify_module();
  llvm::GenericValue code_run();
  int write_object(std::string &);
  ValTypeTuple get_local(std::string s) { return blocks.top()->locals[s]; }
  void set_local(std::string s, llvm::Value *val, llvm::Type *type) {
    blocks.top()->locals[s] = std::make_pair(val, type);
  }
  std::map<std::string, ValTypeTuple> &locals() { return blocks.top()->locals; }
  llvm::BasicBlock *current_block() { return blocks.top()->block; }
  void push_block(llvm::BasicBlock *block) {
    blocks.push(new CodeGenBlock());
    blocks.top()->ret_val = nullptr;
    blocks.top()->block = block;
  }
  void pop_block() {
    CodeGenBlock *top = blocks.top();
    blocks.pop();
    delete top;
  }
  void set_return_value(llvm::Value *value) { blocks.top()->ret_val = value; }
  llvm::Value *get_return_value() { return blocks.top()->ret_val; }
};

#endif
