#ifndef __CODE_GEN_HPP__
#define __CODE_GEN_HPP__

#include <stack>
#include <typeinfo>
#include <iostream>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Verifier.h>

struct CodeGenException : public std::exception {
  std::string message = "Generic code generation exception";
  CodeGenException() {}
  CodeGenException(std::string message) : message(message) {}
  const char *what() const throw() { return message.c_str(); }
};

class NBlock;

extern llvm::LLVMContext LLVM_CTX;
extern llvm::IRBuilder<> BUILDER;

class CodeGenBlock {
public:
  llvm::BasicBlock *block;
  llvm::Value *ret_val;
  std::map<std::string, llvm::Value *> locals;
};

class CodeGenContext {
  std::stack<CodeGenBlock *> blocks;
  llvm::Function *fn_main;

public:
  llvm::Module *module;
  CodeGenContext(std::string module_name = "mod_main") {
    module = new llvm::Module(module_name, LLVM_CTX);
  }

  void code_generate(NBlock &root);
  void print_llvm_ir();
  void print_llvm_ir_to_file(std::string &);
  void verify_module();
  llvm::GenericValue code_run();
  llvm::Value *get_local(std::string s) { return blocks.top()->locals[s]; }
  void set_local(std::string s, llvm::Value *val) {
    blocks.top()->locals[s] = val;
  }
  std::map<std::string, llvm::Value *> &locals() {
    return blocks.top()->locals;
  }
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

void create_core_fns(CodeGenContext &);

#endif
