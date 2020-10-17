/*
 * Stolen directly from
 *   https://raw.githubusercontent.com/lsegal/my_toy_compiler/master/corefn.cpp
 *   at the minute... should probably actually read this at some point
 */

#include "ast.hpp"
#include "codegen.hpp"
#include <iostream>

extern int yyparse();
extern NBlock *programBlock;

llvm::Function *create_fn_printf(CodeGenContext &ctx) {
  std::vector<llvm::Type *> printf_arg_types;
  printf_arg_types.push_back(llvm::Type::getInt8PtrTy(LLVM_CTX));

  llvm::FunctionType *printf_type = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(LLVM_CTX), printf_arg_types, true);

  llvm::Function *func =
      llvm::Function::Create(printf_type, llvm::Function::ExternalLinkage,
                             llvm::Twine("printf"), ctx.module);
  func->setCallingConv(llvm::CallingConv::C);
  return func;
}

void create_fn_echo(CodeGenContext &ctx, llvm::Function *printfFn) {
  std::vector<llvm::Type *> echo_arg_types;
  echo_arg_types.push_back(llvm::Type::getInt64Ty(LLVM_CTX));

  llvm::FunctionType *echo_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(LLVM_CTX), echo_arg_types, false);

  llvm::Function *func =
      llvm::Function::Create(echo_type, llvm::Function::InternalLinkage,
                             llvm::Twine("echo"), ctx.module);
  llvm::BasicBlock *bblock =
      llvm::BasicBlock::Create(LLVM_CTX, "entry", func, 0);
  ctx.push_block(bblock);

  const char *constValue = "%d\n";
  llvm::Constant *format_const =
      llvm::ConstantDataArray::getString(LLVM_CTX, constValue);
  llvm::GlobalVariable *var = new llvm::GlobalVariable(
      *ctx.module,
      llvm::ArrayType::get(llvm::IntegerType::get(LLVM_CTX, 8),
                           strlen(constValue) + 1),
      true, llvm::GlobalValue::PrivateLinkage, format_const, ".str");
  llvm::Constant *zero =
      llvm::Constant::getNullValue(llvm::IntegerType::getInt32Ty(LLVM_CTX));

  std::vector<llvm::Constant *> indices;
  indices.push_back(zero);
  indices.push_back(zero);
  llvm::Constant *var_ref = llvm::ConstantExpr::getGetElementPtr(
      llvm::ArrayType::get(llvm::IntegerType::get(LLVM_CTX, 8),
                           strlen(constValue) + 1),
      var, indices);

  std::vector<llvm::Value *> args;
  args.push_back(var_ref);

  llvm::Function::arg_iterator argsValues = func->arg_begin();
  llvm::Value *toPrint = &*argsValues++;
  toPrint->setName("toPrint");
  args.push_back(toPrint);

  llvm::CallInst *call =
      llvm::CallInst::Create(printfFn, makeArrayRef(args), "", bblock);
  llvm::ReturnInst::Create(LLVM_CTX, bblock);
  ctx.pop_block();
}

void create_core_fns(CodeGenContext &ctx) {
  llvm::Function *fn_printf = create_fn_printf(ctx);
  create_fn_echo(ctx, fn_printf);
}
