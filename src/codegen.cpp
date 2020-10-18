// `ast.hpp` must be before `parser.hpp`...
#include "codegen.hpp"
#include "ast.hpp"
#include "parser.hpp"

llvm::LLVMContext LLVM_CTX;
llvm::IRBuilder<> BUILDER(LLVM_CTX);

llvm::Value *E_LOG_V(const char *str) {
  std::cerr << "E_LOG_V: " << str << std::endl;
  return nullptr;
}

llvm::Type *E_LOG_T(const char *str) {
  std::cerr << "E_LOG_T: " << str << std::endl;
  return nullptr;
}

void CodeGenContext::code_generate(NBlock &root) {
  std::vector<llvm::Type *> arg_types;

  llvm::FunctionType *fn_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(LLVM_CTX), makeArrayRef(arg_types), false);

  fn_main = llvm::Function::Create(fn_type, llvm::GlobalValue::InternalLinkage,
                                   "fn_main", module);
  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, "entry", fn_main, 0);

  push_block(_block);

  BUILDER.SetInsertPoint(_block);
  root.code_generate(*this);  // emit bytecode for the toplevel block
  BUILDER.CreateRet(nullptr); // return `void`

  pop_block();

  std::cout << "\nModule verification:\n" << std::endl;
  llvm::verifyModule(*module, &llvm::outs());
  std::cout << '\n' << std::endl;
}

void CodeGenContext::print_llvm_ir() { module->print(llvm::outs(), nullptr); }

void CodeGenContext::print_llvm_ir_to_file(std::string &filename) {
  std::error_code ec;
  llvm::raw_fd_ostream *ost = new llvm::raw_fd_ostream(filename, ec, llvm::sys::fs::F_None);
  module->print(*ost, nullptr);
  delete ost;
}

/* Executes the AST by running the main function */
llvm::GenericValue CodeGenContext::code_run() {
  llvm::ExecutionEngine *engine =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  engine->finalizeObject();
  std::vector<llvm::GenericValue> no_args;
  llvm::GenericValue v = engine->runFunction(fn_main, no_args);
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
  return E_LOG_T("Unknown variable type");
}

/* -------- Types  -------- */

llvm::Value *NInteger::code_generate(CodeGenContext &ctx) {
  std::cout << "IS AN INTEGER" << std::endl;
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLVM_CTX), val, true);
}

llvm::Value *NFloat::code_generate(CodeGenContext &ctx) {
  std::cout << "IS A FLOAT" << std::endl;
  return llvm::ConstantFP::get(llvm::Type::getDoubleTy(LLVM_CTX), val);
}

// TODO:
llvm::Value *NString::code_generate(CodeGenContext &ctx) {
  return E_LOG_V("String not yet implemented");
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

llvm::Value *NBinaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Type *double_type = llvm::Type::getDoubleTy(LLVM_CTX);
  llvm::Type *integer_type = llvm::Type::getInt64Ty(LLVM_CTX);

  llvm::Value *_lhs = lhs.code_generate(ctx);
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_lhs || !_rhs)
    return E_LOG_V("Couldn't generate code for binary comparison");

  // Judging operands by only the lhs...
  llvm::Type *_lhs_type = _lhs->getType();
  llvm::Type *_rhs_type = _rhs->getType();

  if (_lhs_type == double_type && _rhs_type == integer_type) {
    _rhs = BUILDER.CreateCast(llvm::Instruction::UIToFP, _rhs, double_type);
    _rhs_type = double_type;
  }
  if (_lhs_type == integer_type && _rhs_type == double_type) {
    _lhs = BUILDER.CreateCast(llvm::Instruction::UIToFP, _lhs, double_type);
    _lhs_type = double_type;
  }

  // TODO: Strings...
  switch (op) {
  case OP_PLUS:
    return BUILDER.CreateAdd(_lhs, _rhs, "add");
  case OP_MINUS:
    return BUILDER.CreateSub(_lhs, _rhs, "sub");
  case OP_MULTIPLIED_BY:
    return BUILDER.CreateMul(_lhs, _rhs, "mul");
  case OP_DIVIDED_BY:
    return BUILDER.CreateSDiv(_lhs, _rhs, "div");
  case OP_MODULO: // ?
    return BUILDER.CreateSRem(_lhs, _rhs, "srem_mod");
  case OP_ALSO:
    return BUILDER.CreateAnd(_lhs, _rhs, "also");
  case OP_ALTERNATIVELY:
    return BUILDER.CreateOr(_lhs, _rhs, "alternatively");
  case OP_EQUAL_TO: // Order matters?
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpOEQ(_lhs, _rhs, "f_equal");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpEQ(_lhs, _rhs, "i_equal");
    return E_LOG_V("No relevant type found for binary EQ");
  case OP_NOT_EQUAL_TO:
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpONE(_lhs, _rhs, "f_not_equal");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpNE(_lhs, _rhs, "i_not_equal");
    return E_LOG_V("No relevant type found for binary NE");
  case OP_LESS_THAN:
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpOLT(_lhs, _rhs, "f_less_than");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpSLT(_lhs, _rhs, "i_less_than");
    return E_LOG_V("No relevant type found for binary LT");
  case OP_LESS_THAN_EQUAL_TO:
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpOLE(_lhs, _rhs, "f_less_than_or_equal_to");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpSLE(_lhs, _rhs, "i_less_than_or_equal_to");
    return E_LOG_V("No relevant type found for binary LE");
  case OP_MORE_THAN:
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpOGT(_lhs, _rhs, "f_more_than");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpSGT(_lhs, _rhs, "i_more_than");
    return E_LOG_V("No relevant type found for binary GT");
  case OP_MORE_THAN_EQUAL_TO:
    if (_lhs_type == double_type)
      return BUILDER.CreateFCmpOGE(_lhs, _rhs, "f_more_than_or_equal_to");
    if (_lhs_type == integer_type)
      return BUILDER.CreateICmpSGE(_lhs, _rhs, "i_more_than_or_equal_to");
    return E_LOG_V("No relevant type found for binary GE");
  default:
    throw CodeGenException("Invalid binary operator");
  }
}

/* ------ Chunks ------*/
llvm::Value *NBlock::code_generate(CodeGenContext &ctx) {
  llvm::Value *last = nullptr;
  NStatementList::const_iterator it;
  llvm::BasicBlock *_current_block = BUILDER.GetInsertBlock();
  for (it = stmts.begin(); it != stmts.end(); it++) {
    last = (*it)->code_generate(ctx);
    BUILDER.SetInsertPoint(_current_block);
  }
  return last;
}

llvm::Value *NAssignment::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = ctx.get_local(lhs.val);
  if (!_lhs)
    throw CodeGenException("Variable " + lhs.val +
                           " not defined in current block");
  return BUILDER.CreateStore(rhs.code_generate(ctx), _lhs);
}

llvm::Value *NRead::code_generate(CodeGenContext &ctx) {
  return E_LOG_V("Read not yet implemented");
}
llvm::Value *NWrite::code_generate(CodeGenContext &ctx) {
  return E_LOG_V("Write not yet implemented");
}

llvm::Value *NReturnStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_ret = ctx.get_return_value();
  if (!_ret)
    throw CodeGenException("Return value not exprection for current function");
  return BUILDER.CreateStore(exp.code_generate(ctx), _ret);
}

llvm::Value *NExpressionStatement::code_generate(CodeGenContext &ctx) {
  return exp.code_generate(ctx);
}

/**
 * Zero initialization, kind of unnecessary but, hey
 *
 * TODO: String -> ""
 */
static llvm::Value *zero_value_for(llvm::Type *type) {
  llvm::Type *double_type = llvm::Type::getDoubleTy(LLVM_CTX);
  llvm::Type *integer_type = llvm::Type::getInt64Ty(LLVM_CTX);
  if (type == double_type)
    return llvm::ConstantFP::get(double_type, 0.0);
  if (type == integer_type)
    return llvm::ConstantInt::get(integer_type, 0, true);
  // if (type == string)...
  return E_LOG_V("Unknown variable type");
}

llvm::Value *NVariableDeclaration::code_generate(CodeGenContext &ctx) {
  llvm::Type *_lhs_type = type_of(type);
  llvm::Value *_lhs = BUILDER.CreateAlloca(_lhs_type, 0, lhs.val.c_str());
  ctx.set_local(lhs.val, _lhs);
  if (rhs)
    BUILDER.CreateStore(rhs->code_generate(ctx), ctx.get_local(lhs.val));
  else // zero-initialize
    BUILDER.CreateStore(zero_value_for(_lhs_type), ctx.get_local(lhs.val));
  return _lhs;
}

llvm::Value *NFunctionDeclaration::code_generate(CodeGenContext &ctx) {
  std::vector<llvm::Type *> arg_types;
  NVariableList::const_iterator it;

  // For arrayRef
  for (it = args.begin(); it != args.end(); it++)
    arg_types.push_back(type_of((*it)->type));

  llvm::FunctionType *fn_type = llvm::FunctionType::get(
      type_of(type), llvm::makeArrayRef(arg_types), false);

  llvm::Function *fn = llvm::Function::Create(
      fn_type, llvm::GlobalValue::InternalLinkage, id.val.c_str(), ctx.module);

  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, id.val + "__entry", fn, 0);

  ctx.push_block(_block);

  BUILDER.SetInsertPoint(_block);
  llvm::Value *ret_val = BUILDER.CreateAlloca(fn_type, 0, id.val + "__ret_val");
  ctx.set_return_value(ret_val);

  llvm::Function::arg_iterator arg_it = fn->arg_begin();
  for (it = args.begin(); it != args.end(); it++) {
    (*it)->code_generate(ctx);
    llvm::Value *_arg_value = &*arg_it++;
    _arg_value->setName((*it)->lhs.val.c_str());
  }

  block.code_generate(ctx);

  BUILDER.CreateRet(ret_val);

  ctx.pop_block();

  // llvm::verifyFunction(*fn, &llvm::outs());

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
    return E_LOG_V("Invalid condition for `if`");
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
    return E_LOG_V("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(false), "while_cond");

  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_while =
      llvm::BasicBlock::Create(LLVM_CTX, "while_block", _fn);

  BUILDER.CreateBr(_while);
  BUILDER.SetInsertPoint(_while);

  if (!block.code_generate(ctx))
    return E_LOG_V("Invalid while block");

  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "while_cnt", _fn);

  BUILDER.CreateCondBr(_cond, _while, _after);
  BUILDER.SetInsertPoint(_after);

  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(LLVM_CTX));
}

llvm::Value *NUntilStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    return E_LOG_V("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(true), "until_cond");

  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_until =
      llvm::BasicBlock::Create(LLVM_CTX, "until_block", _fn);

  BUILDER.CreateBr(_until);
  BUILDER.SetInsertPoint(_until);

  if (!block.code_generate(ctx))
    return E_LOG_V("Invalid while block");

  llvm::BasicBlock *_end = BUILDER.GetInsertBlock();
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "until_cnt", _fn);

  BUILDER.CreateCondBr(_cond, _until, _after);
  BUILDER.SetInsertPoint(_after);

  return llvm::Constant::getNullValue(llvm::Type::getDoubleTy(LLVM_CTX));
}
