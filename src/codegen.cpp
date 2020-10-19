/**
 * Note:
 * - `ast.hpp` must be before `parser.hpp`...
 *
 * Todo:
 * - Cleanup `get_current_block` functions and the like, use `BUILDER`
 */
#include "codegen.hpp"
#include "ast.hpp"
#include "parser.hpp"

llvm::LLVMContext LLVM_CTX;
llvm::IRBuilder<> BUILDER(LLVM_CTX);

llvm::Type *DOUBLE_TYPE = llvm::Type::getDoubleTy(LLVM_CTX);
llvm::Type *STRING_TYPE = llvm::Type::getInt8PtrTy(LLVM_CTX);
llvm::Type *INTEGER_TYPE = llvm::Type::getInt64Ty(LLVM_CTX);

llvm::Value *E_LOG_V(const char *str) {
  std::cerr << "E_LOG_V: " << str << std::endl;
  return nullptr;
}

llvm::Type *E_LOG_T(const char *str) {
  std::cerr << "E_LOG_T: " << str << std::endl;
  return nullptr;
}

llvm::Constant *get_i8_str_ptr(char const *str, llvm::Twine const &twine) {
  return BUILDER.CreateGlobalStringPtr(str, twine);
}

CodeGenContext::CodeGenContext(std::string module_name) {
  module = new llvm::Module(module_name, LLVM_CTX);
  printf_function = create_fn_printf();
}

llvm::Function *CodeGenContext::create_fn_printf() {
  std::vector<llvm::Type *> printf_arg_types;
  printf_arg_types.push_back(llvm::Type::getInt8PtrTy(LLVM_CTX));

  llvm::FunctionType *printf_type = llvm::FunctionType::get(
      llvm::Type::getInt32Ty(LLVM_CTX), printf_arg_types, true);

  llvm::Function *func =
      llvm::Function::Create(printf_type, llvm::Function::ExternalLinkage,
                             llvm::Twine("printf"), module);
  func->setCallingConv(llvm::CallingConv::C);
  return func;
}

void CodeGenContext::code_generate(NBlock &root) {
  std::vector<llvm::Type *> arg_types;

  llvm::FunctionType *fn_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(LLVM_CTX), makeArrayRef(arg_types), false);

  fn_main = llvm::Function::Create(fn_type, llvm::GlobalValue::InternalLinkage,
                                   "main", module);
  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, "entry", fn_main, 0);

  push_block(_block);

  BUILDER.SetInsertPoint(_block);

  fmt_specifiers.insert({"numeric", get_i8_str_ptr("%d", "numeric_fmt_spc")});
  fmt_specifiers.insert({"string", get_i8_str_ptr("%s", "string_fmt_spc")});

  root.code_generate(*this);  // emit bytecode for the toplevel block
  BUILDER.CreateRet(nullptr); // return `void`

  pop_block();
}

void CodeGenContext::verify_module() {
  llvm::verifyModule(*module, &llvm::outs());
}

void CodeGenContext::print_llvm_ir() { module->print(llvm::outs(), nullptr); }

void CodeGenContext::print_llvm_ir_to_file(std::string &filename) {
  std::error_code ec;
  llvm::raw_fd_ostream *ost =
      new llvm::raw_fd_ostream(filename, ec, llvm::sys::fs::F_None);
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
    return llvm::Type::getInt8PtrTy(LLVM_CTX); // TODO: Work out string type
  return E_LOG_T("Unknown variable type");
}

/* -------- Types  -------- */

llvm::Value *NInteger::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantInt::get(llvm::Type::getInt64Ty(LLVM_CTX), val, true);
}

llvm::Value *NFloat::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantFP::get(llvm::Type::getDoubleTy(LLVM_CTX), val);
}

void replace_all(std::string &input, const std::string &from,
                 const std::string &to) {
  if (!from.empty()) {
    size_t start_pos = 0;
    while ((start_pos = input.find(from, start_pos)) != std::string::npos) {
      input.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }
}

void process_escape_chars(std::string &input) {
  replace_all(input, "\\n", "\n");
  replace_all(input, "\\r", "\r");
  replace_all(input, "\\t", "\t");
}

llvm::Value *NString::code_generate(CodeGenContext &ctx) {
  process_escape_chars(val);
  return get_i8_str_ptr(val.c_str(), "l_str");
}

llvm::Value *NIdentifier::code_generate(CodeGenContext &ctx) {
  if (ctx.locals().find(val) == ctx.locals().end()) {
    std::string msg = "Identifier " + val + " not found in current context";
    return E_LOG_V(msg.c_str());
  }
  return new llvm::LoadInst(ctx.locals()[val], "", false,
                            BUILDER.GetInsertBlock());
}

/* ----- Operative expressions ------ */

llvm::Value *NUnaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_rhs)
    return E_LOG_V("Couldn't generate IR for RHS");

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
  for (it = stmts.begin(); it != stmts.end(); it++) {
    last = (*it)->code_generate(ctx);
  }
  return last;
}

llvm::Value *NAssignment::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = ctx.get_local(lhs.val);
  if (!_lhs)
    throw CodeGenException("Variable " + lhs.val +
                           " not defined in current block");
  llvm::Value *_rhs = rhs.code_generate(ctx);
  // TODO: if (_lhs->getType() == STRING_TYPE && _rhs->getType() != STRING_TYPE)
  return BUILDER.CreateStore(rhs.code_generate(ctx), _lhs);
}

// Doing nothing `to` at the minute, all to stdout
llvm::Value *NWrite::code_generate(CodeGenContext &ctx) {
  // Todo: Something for `string_type`
  llvm::Value *_exp = exp.code_generate(ctx);
  llvm::Type *_exp_type = _exp->getType();
  if (_exp_type == DOUBLE_TYPE || _exp_type == INTEGER_TYPE) {
    llvm::SmallVector<llvm::Value *, 2> printf_args;
    printf_args.push_back(ctx.fmt_specifiers.at("numeric"));
    printf_args.push_back(_exp);
    return BUILDER.CreateCall(ctx.printf_function, printf_args);
  } else if (_exp_type == STRING_TYPE) {
    llvm::SmallVector<llvm::Value *, 2> printf_args;
    printf_args.push_back(ctx.fmt_specifiers.at("string"));
    printf_args.push_back(_exp);
    return BUILDER.CreateCall(ctx.printf_function, printf_args);
  }
  return E_LOG_V("Write not yet implemented");
}

llvm::Value *NRead::code_generate(CodeGenContext &ctx) {
  return E_LOG_V("Read not yet implemented");
}

llvm::Value *NReturnStatement::code_generate(CodeGenContext &ctx) {
  return BUILDER.CreateRet(exp.code_generate(ctx));
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
  if (type == DOUBLE_TYPE)
    return llvm::ConstantFP::get(DOUBLE_TYPE, 0.0);
  if (type == INTEGER_TYPE)
    return llvm::ConstantInt::get(INTEGER_TYPE, 0, true);
  if (type == STRING_TYPE)
    return get_i8_str_ptr("", "str_init_val");
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

  llvm::BasicBlock *_current_block = BUILDER.GetInsertBlock();

  std::vector<llvm::Type *> arg_types;
  NVariableList::const_iterator it;

  for (it = args.begin(); it != args.end(); it++)
    arg_types.push_back(type_of((*it)->type));

  llvm::FunctionType *_fn_type = llvm::FunctionType::get(
      type_of(type), llvm::makeArrayRef(arg_types), false);

  llvm::Function *_fn = llvm::Function::Create(
      _fn_type, llvm::GlobalValue::InternalLinkage, id.val.c_str(), ctx.module);

  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, id.val + "__entry", _fn, 0);

  ctx.push_block(_block);

  BUILDER.SetInsertPoint(_block);

  llvm::Function::arg_iterator arg_it = _fn->arg_begin();

  for (it = args.begin(); it != args.end(); it++) {
    // llvm::Value *arg_val = (*it)->code_generate(ctx);
    llvm::Value *_arg_value = arg_it++;
    _arg_value->setName((*it)->lhs.val.c_str());
    llvm::Value *_in_f_arg =
        BUILDER.CreateAlloca(type_of((*it)->type), 0, (*it)->lhs.val);
    BUILDER.CreateStore(_arg_value, _in_f_arg);
    ctx.set_local((*it)->lhs.val, _in_f_arg);
  }

  block.code_generate(ctx);

  ctx.pop_block();

  BUILDER.SetInsertPoint(_current_block);

  return _fn;
}

llvm::Value *NFunctionCall::code_generate(CodeGenContext &ctx) {
  llvm::Function *fn = ctx.module->getFunction(id.val.c_str());

  if (!fn)
    throw CodeGenException("Attempted call on unknown function");

  std::vector<llvm::Value *> _args;

  NExpressionList::const_iterator it;
  for (it = args.begin(); it != args.end(); it++)
    _args.push_back((*it)->code_generate(ctx));

  return BUILDER.CreateCall(fn, _args, "call_tmp");
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
  BUILDER.CreateBr(_aftr);

  // Emit `else` block
  BUILDER.SetInsertPoint(_else);
  _fn->getBasicBlockList().push_back(_else);
  if (els)
    els->code_generate(ctx);
  BUILDER.CreateBr(_aftr);

  BUILDER.SetInsertPoint(_aftr);
  _fn->getBasicBlockList().push_back(_aftr);

  // NOTE: Nothing specific to return and handles block movement internally
  return nullptr;
}

llvm::Value *NElseStatement::code_generate(CodeGenContext &ctx) {
  return block.code_generate(ctx);
}

llvm::Value *NWhileStatement::code_generate(CodeGenContext &ctx) {
  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_while =
      llvm::BasicBlock::Create(LLVM_CTX, "while_cond", _fn);
  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, "while_block", _fn);
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "while_aftr", _fn);

  BUILDER.CreateBr(_while);
  BUILDER.SetInsertPoint(_while);
  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    return E_LOG_V("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(false), "while_cond");
  BUILDER.CreateCondBr(_cond, _block, _after);

  BUILDER.SetInsertPoint(_block);
  if (!block.code_generate(ctx))
    return E_LOG_V("Invalid while block");
  BUILDER.CreateBr(_while);

  BUILDER.SetInsertPoint(_after);

  return nullptr;
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
