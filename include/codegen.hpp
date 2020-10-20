#ifndef __CODE_GEN_HPP__
#define __CODE_GEN_HPP__

#include <iostream>
#include <stack>
#include <typeinfo>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
// #include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/CallingConv.h>
// #include <llvm/IR/DerivedTypes.h>
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

class CodeGenBlock {
public:
  llvm::BasicBlock *block;
  llvm::Value *ret_val;
  std::map<std::string, ValTypeTuple> locals;
};

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
