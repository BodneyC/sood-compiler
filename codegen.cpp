// `ast.hpp` must be before `parser.hpp`...
#include "ast.hpp"
#include "parser.hpp"
#include "codegen.hpp"

llvm::Value *E_LOG(const char *str) {
  std::cerr << str << std::endl;
  return nullptr;
}

void CodeGenContext::code_generate(NBlock &root) {
  /* Create the top level interpreter function to call as entry */
  std::vector<llvm::Type *> arg_types;
  llvm::FunctionType *fType = llvm::FunctionType::get(
      llvm::Type::getVoidTy(LLVM_CTX), makeArrayRef(arg_types), false);
  f_main = llvm::Function::Create(fType, llvm::GlobalValue::InternalLinkage,
                                  "main", module);
  llvm::BasicBlock *bblock =
      llvm::BasicBlock::Create(LLVM_CTX, "entry", f_main, 0);

  push_block(bblock);
  root.code_generate(*this); /* emit bytecode for the toplevel block */
  llvm::ReturnInst::Create(LLVM_CTX, bblock);
  pop_block();

  // module->dump();

  llvm::legacy::PassManager pm;
  pm.add(createPrintModulePass(llvm::outs()));
  pm.run(*module);
}

/* Executes the AST by running the main function */
llvm::GenericValue CodeGenContext::run_code() {
  llvm::ExecutionEngine *engine =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  engine->finalizeObject();
  std::vector<llvm::GenericValue> no_args;
  llvm::GenericValue v = engine->runFunction(f_main, no_args);
  return v;
}

/* Returns an LLVM type based on the identifier */
static llvm::Type *type_of(const NIdentifier &type) {
  if (type.val == "integer")
    return llvm::Type::getInt64Ty(LLVM_CTX);
  if (type.val == "float")
    return llvm::Type::getDoubleTy(LLVM_CTX);
  if (type.val == "string")
    return nullptr; // TODO: Work out string type
  return nullptr;
}

/* -------- Types  -------- */

llvm::Value *NInteger::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLVM_CTX), val, true);
}

llvm::Value *NFloat::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantFP::get(llvm::Type::getDoubleTy(LLVM_CTX), val);
}

// TODO:
llvm::Value *NString::code_generate(CodeGenContext &ctx) {
  return E_LOG("String not yet implemented");
}

llvm::Value *NIdentifier::code_generate(CodeGenContext &ctx) {
  if (ctx.locals().find(val) == ctx.locals().end()) {
    return nullptr;
  }
  return new llvm::LoadInst(ctx.locals()[val], "", false, ctx.current_block());
}

/* ----- Operative expressions ------ */

llvm::Value *NUnaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_rhs)
    return nullptr;

  switch (op) {
  case TNOT:
    return BUILDER.CreateNot(_rhs);
  case TNEG:
    return BUILDER.CreateFNeg(_rhs);
  default:
    throw CodeGenException("Invalid unary operator");
  }
}

/*
 * TODO: Need to know the type of `NIdentifier` for the float variables
 * (`FMul`...)
 */
llvm::Value *NBinaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = lhs.code_generate(ctx);
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_lhs || !_rhs)
    return nullptr;

  switch (op) {
  case TPLS:
    return BUILDER.CreateFAdd(_lhs, _rhs, "add");
  case TMNS:
    return BUILDER.CreateFSub(_lhs, _rhs, "sub");
  case TMUL:
    return BUILDER.CreateFMul(_lhs, _rhs, "mul");
  case TDIV:
    return BUILDER.CreateFDiv(_lhs, _rhs, "div");
  case TMOD: // ?
    return BUILDER.CreateSRem(_lhs, _rhs, "srem_mod");
  case TEQ: // Order matters?
    return BUILDER.CreateFCmpOEQ(_lhs, _rhs, "equal");
  case TNE:
    return BUILDER.CreateFCmpONE(_lhs, _rhs, "not_equal");
  case TLT:
    return BUILDER.CreateFCmpOLT(_lhs, _rhs, "less_than");
  case TLE:
    return BUILDER.CreateFCmpOLE(_lhs, _rhs, "less_than_or_equal_to");
  case TMT:
    return BUILDER.CreateFCmpOGT(_lhs, _rhs, "more_than");
  case TME:
    return BUILDER.CreateFCmpOGE(_lhs, _rhs, "more_than_or_equal_to");
  case TALS:
    return BUILDER.CreateAnd(_lhs, _rhs, "also");
  case TALT:
    return BUILDER.CreateOr(_lhs, _rhs, "alternatively");
  default:
    throw CodeGenException("Invalid binary operator");
  }
}

/* ------ Chunks ------*/
llvm::Value *NBlock::code_generate(CodeGenContext &ctx) {
  llvm::Value *last = nullptr;
  NStatementList::const_iterator it;
  for (it = stmts.begin(); it != stmts.end(); it++)
    last = (**it).code_generate(ctx);
  return last;
}

llvm::Value *NAssignment::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = ctx.get_local(lhs.val);
  if (!_lhs)
    throw CodeGenException("Variable " + lhs.val +
                           " not defined in current block");
  return BUILDER.CreateStore(_lhs, rhs.code_generate(ctx));
}

llvm::Value *NRead::code_generate(CodeGenContext &ctx) {
  return E_LOG("Read not yet implemented");
}
llvm::Value *NWrite::code_generate(CodeGenContext &ctx) {
  return E_LOG("Write not yet implemented");
}

llvm::Value *NReturnStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_ret = exp.code_generate(ctx);
  ctx.set_return_value(_ret);
  return BUILDER.CreateRet(_ret);
}

llvm::Value *NExpressionStatement::code_generate(CodeGenContext &ctx) {
  return exp.code_generate(ctx);
}

llvm::Value *NVariableDeclaration::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = BUILDER.CreateAlloca(type_of(type), 0, lhs.val.c_str());
  ctx.set_local(lhs.val, _lhs);
  if (rhs != nullptr)
    BUILDER.CreateStore(ctx.get_local(lhs.val), rhs->code_generate(ctx));
  return _lhs;
}

llvm::Value *NFunctionDeclaration::code_generate(CodeGenContext &ctx) {
  std::vector<llvm::Type *> arg_types;
  NVariableList::const_iterator it;

  // For arrayRef
  for (it = args.begin(); it != args.end(); it++)
    arg_types.push_back(type_of((*it)->type));

  llvm::FunctionType *fn_type =
      llvm::FunctionType::get(type_of(type), makeArrayRef(arg_types), false);

  llvm::Function *fn = llvm::Function::Create(
      fn_type, llvm::GlobalValue::InternalLinkage, id.val.c_str(), ctx.module);

  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, id.val + "__entry", fn, 0);

  ctx.push_block(_block);

  llvm::Function::arg_iterator arg_it = fn->arg_begin();
  for (it = args.begin(); it != args.end(); it++) {
    (*it)->code_generate(ctx);
    llvm::Value *_arg_value = &*arg_it++;
    _arg_value->setName((*it)->lhs.val.c_str());
  }

  block.code_generate(ctx);

  ctx.pop_block();

  return fn;
}

llvm::Value *NFunctionCall::code_generate(CodeGenContext &ctx) {
  llvm::Function *fn = ctx.module->getFunction(id.val.c_str());

  if (!fn)
    throw CodeGenException("Attempted call on unknown function");

  std::vector<llvm::Value *> _args;

  NExpressionList::const_iterator it;
  for (it = args.begin(); it != args.end(); it++)
    _args.push_back((**it).code_generate(ctx));

  return BUILDER.CreateCall(fn, _args);
}

/* ------ constructs ------ */

llvm::Value *NIfStatement::code_generate(CodeGenContext &ctx) {

  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    return E_LOG("Invalid condition for `if`");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(0), "if_cond");

  // Get current block
  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();

  // All `if`s are `if-else`, but with a blank `else`
  llvm::BasicBlock *_then = llvm::BasicBlock::Create(LLVM_CTX, "if_then", _fn);
  llvm::BasicBlock *_else = llvm::BasicBlock::Create(LLVM_CTX, "if_else");
  llvm::BasicBlock *_aftr = llvm::BasicBlock::Create(LLVM_CTX, "if_cnt");
  BUILDER.CreateCondBr(_cond, _then, _else);

  // Start of the `_then` if condition true
  BUILDER.SetInsertPoint(_then);
  llvm::Value *_then_val = block.code_generate(ctx);
  if (!_then_val)
    throw CodeGenException("Could not generate `then` block");

  // Setup `after-if` cont. point
  BUILDER.CreateBr(_aftr);
  _then = BUILDER.GetInsertBlock();

  // Emit `else` block
  _fn->getBasicBlockList().push_back(_else);

  BUILDER.SetInsertPoint(_else);
  llvm::Value *_els_val = nullptr;
  if (els) {
    _els_val = els->code_generate(ctx);
    if (!_els_val)
      throw CodeGenException("`else` block exists but generated no code");
  }

  BUILDER.CreateBr(_aftr);
  _else = BUILDER.GetInsertBlock();

  _fn->getBasicBlockList().push_back(_aftr);
  BUILDER.SetInsertPoint(_aftr);
  llvm::PHINode *phi =
      BUILDER.CreatePHI(llvm::Type::getDoubleTy(LLVM_CTX), 2, "if_phi_tmp");

  phi->addIncoming(_then_val, _then);
  phi->addIncoming(_els_val, _else);
  return phi;
}

llvm::Value *NElseStatement::code_generate(CodeGenContext &ctx) {
  return block.code_generate(ctx);
}

llvm::Value *NWhileStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    return E_LOG("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(false), "while_cond");

  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_while =
      llvm::BasicBlock::Create(LLVM_CTX, "while_block", _fn);

  BUILDER.CreateBr(_while);
  BUILDER.SetInsertPoint(_while);

  if (!block.code_generate(ctx))
    return E_LOG("Invalid while block");

  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "while_cnt", _fn);

  BUILDER.CreateCondBr(_cond, _while, _after);
  BUILDER.SetInsertPoint(_after);

  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(LLVM_CTX));
}

llvm::Value *NUntilStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    return E_LOG("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(true), "until_cond");

  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_until =
      llvm::BasicBlock::Create(LLVM_CTX, "until_block", _fn);

  BUILDER.CreateBr(_until);
  BUILDER.SetInsertPoint(_until);

  if (!block.code_generate(ctx))
    return E_LOG("Invalid while block");

  llvm::BasicBlock *_end = BUILDER.GetInsertBlock();
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "until_cnt", _fn);

  BUILDER.CreateCondBr(_cond, _until, _after);
  BUILDER.SetInsertPoint(_after);

  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(LLVM_CTX));
}
