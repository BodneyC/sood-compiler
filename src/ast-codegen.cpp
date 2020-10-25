#include "ast.hpp"
#include "codegen.hpp"
#include "parser.hpp"

/** Returns an LLVM type based on the identifier */
static llvm::Type *type_of(const NIdentifier &type) {
  if (type.val == "integer")
    return llvm::Type::getInt64Ty(LLVM_CTX);
  if (type.val == "float")
    return llvm::Type::getDoubleTy(LLVM_CTX);
  if (type.val == "string")
    return llvm::Type::getInt8PtrTy(LLVM_CTX);
  throw CodeGenException("Unknown variable type");
}

/* -------- Types  -------- */

/**
 * Construct: Method
 * Name: NInteger::code_generate
 * Desc: Create a variable of constant integer type (see `INTEGER_TYPE`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NInteger::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantInt::get(INTEGER_TYPE, val, true);
}

/**
 * Construct: Method
 * Name: NFloat::code_generate
 * Desc: Create a variable of constant floating type (see `DOUBLE_TYPE`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NFloat::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantFP::get(DOUBLE_TYPE, val);
}

void replace_all(std::string &input, const std::string &from,
                 const std::string &to) {
  if (!from.empty()) {
    size_t start_pos = 0;
    while ((start_pos = input.find(from, start_pos)) != std::string::npos) {
      if (!(start_pos > 1 && input[start_pos - 1] == '\\'))
        input.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }
}

/**
 * Construct: Function
 * Name: process_escape_chars
 * Desc: The strings read by the lexer appends chars to a string, so any
 *   escaped characters are overlooked (viewed as `\\n` instead of `\n`, for
 *   example) so this simply replaces all instances of common escape character
 *   which are separated into their single character
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - The string "\\n" can be used without being replacement to newline, this
 *     will instead be replaced to `\n`
 */
void process_escape_chars(std::string &input) {
  replace_all(input, "\\n", "\n");
  replace_all(input, "\\r", "\r");
  replace_all(input, "\\t", "\t");
  replace_all(input, "\\\\", "\\");
}

/**
 * Construct: Method
 * Name: NString::code_generate
 * Desc: Creates a global string with the contents of the `val` member and
 *   return a poitner to it
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NString::code_generate(CodeGenContext &ctx) {
  process_escape_chars(val);
  return get_i8_str_ptr(val.c_str(), "l_str");
}

/**
 * Construct: Method
 * Name: NIdentifier::code_generate
 * Desc: This is a reference to a variable and not a variable delcaration, so
 *   first we must check to see if the identifier exists in the current
 *   context, if not, an exception is thrown.  If the identifier does exist,
 *   return the llvm::Value pointer to that local
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NIdentifier::code_generate(CodeGenContext &ctx) {
  if (ctx.locals().find(val) == ctx.locals().end()) {
    std::string msg = "Identifier " + val + " not found in current context";
    throw CodeGenException(msg.c_str());
  }
  ValTypeTuple _ident = ctx.get_local(val);
  return BUILDER.CreateLoad(std::get<llvm::Value *>(_ident), "_val_load");
}

/* ----- Operative expressions ------ */

/**
 * Construct: Method
 * Name: NUnaryExpression::code_generate
 * Desc: Creates a unary operation applied to the relevant RHS using the
 *   global LLVM IR builder (see `BUILDER`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NUnaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_rhs)
    throw CodeGenException("Couldn't generate IR for RHS");

  switch (op) {
  case TNOT:
    return BUILDER.CreateNot(_rhs, "_rhs_not");
  case TNEG:
    return BUILDER.CreateFNeg(_rhs, "_rhs_neg");
  default:
    throw CodeGenException("Invalid unary operator");
  }
}

/**
 * Construct: Method
 * Name: NUnaryExpression::code_generate
 * Desc: Creates a binary operation between the relevant LHS and RHS, there
 *   is some rudimentary type casting between integer and floating point
 *   number if the types of `lhs` and `rhs` differ. Operation is created
 *   using the global LLVM IR builder (see `BUILDER`)
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Currently, any boolean or arithmetic expression applied to a string
 *     will fail, this is a future task
 */
llvm::Value *NBinaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_lhs = lhs.code_generate(ctx);
  llvm::Value *_rhs = rhs.code_generate(ctx);

  if (!_lhs || !_rhs)
    throw CodeGenException("Couldn't generate code for binary comparison");

  // Judging operands by only the lhs...
  llvm::Type *_lhs_type = _lhs->getType();
  llvm::Type *_rhs_type = _rhs->getType();

  if (_lhs_type == DOUBLE_TYPE && _rhs_type == INTEGER_TYPE) {
    _rhs = BUILDER.CreateCast(llvm::Instruction::UIToFP, _rhs, DOUBLE_TYPE,
                              "_rhs_cast");
    _rhs_type = DOUBLE_TYPE;
  }
  if (_lhs_type == INTEGER_TYPE && _rhs_type == DOUBLE_TYPE) {
    _lhs = BUILDER.CreateCast(llvm::Instruction::UIToFP, _lhs, DOUBLE_TYPE,
                              "_lhs_cast");
    _lhs_type = DOUBLE_TYPE;
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
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpOEQ(_lhs, _rhs, "f_equal");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpEQ(_lhs, _rhs, "i_equal");
    throw CodeGenException("No relevant type found for binary EQ");
  case OP_NOT_EQUAL_TO:
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpONE(_lhs, _rhs, "f_not_equal");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpNE(_lhs, _rhs, "i_not_equal");
    throw CodeGenException("No relevant type found for binary NE");
  case OP_LESS_THAN:
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpOLT(_lhs, _rhs, "f_less_than");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpSLT(_lhs, _rhs, "i_less_than");
    throw CodeGenException("No relevant type found for binary LT");
  case OP_LESS_THAN_EQUAL_TO:
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpOLE(_lhs, _rhs, "f_less_than_or_equal_to");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpSLE(_lhs, _rhs, "i_less_than_or_equal_to");
    throw CodeGenException("No relevant type found for binary LE");
  case OP_MORE_THAN:
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpOGT(_lhs, _rhs, "f_more_than");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpSGT(_lhs, _rhs, "i_more_than");
    throw CodeGenException("No relevant type found for binary GT");
  case OP_MORE_THAN_EQUAL_TO:
    if (_lhs_type == DOUBLE_TYPE)
      return BUILDER.CreateFCmpOGE(_lhs, _rhs, "f_more_than_or_equal_to");
    if (_lhs_type == INTEGER_TYPE)
      return BUILDER.CreateICmpSGE(_lhs, _rhs, "i_more_than_or_equal_to");
    throw CodeGenException("No relevant type found for binary GE");
  default:
    throw CodeGenException("Invalid binary operator");
  }
}

/* ------ Chunks ------*/

/**
 * Construct: Method
 * Name: NBlock::code_generate
 * Desc: Iterates over the statements of the block (see `NBlock` : `stmts`)
 *   and calls their respective `code_generate` methods
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NBlock::code_generate(CodeGenContext &ctx) {
  llvm::Value *last = nullptr;
  NStatementList::const_iterator it;
  for (it = stmts.begin(); it != stmts.end(); it++) {
    last = (*it)->code_generate(ctx);
  }
  return last;
}

/**
 * Construct: Function
 * Name: cast_relevantly
 * Desc: Casts the RHS value to the type of LHS for use in an assignment
 * Args:
 *   - _rhs: The LLVM value of the RHS expression
 *   - _lhs_typle: The value/type tuple from the locals of the `CodeGenBlock`
 * Notes:
 *   - Currently, this function does nothing with string and attempts to
 *     assign an integer to a string or vice versa will fail, this is a future
 *     task
 */
llvm::Value *cast_relevantly(llvm::Value *_rhs, ValTypeTuple _lhs_tuple) {
  llvm::Value *_lhs;
  llvm::Type *_lhs_type;
  std::tie(_lhs, _lhs_type) = _lhs_tuple;

  llvm::Type *_rhs_type = _rhs->getType();

  /**
   * Any casting here is on the RHS to match the type of the LHS, we're not
   *   to operate on both independently but assign the expression to the
   *   already know LHS
   */
  if (_lhs_type == DOUBLE_TYPE && _rhs_type == INTEGER_TYPE)
    _rhs = BUILDER.CreateCast(llvm::Instruction::UIToFP, _rhs, DOUBLE_TYPE,
                              "_rhs_cast_to_double");

  if (_lhs_type == INTEGER_TYPE && _rhs_type == DOUBLE_TYPE)
    _rhs = BUILDER.CreateCast(llvm::Instruction::FPToSI, _lhs, INTEGER_TYPE,
                              "_rhs_cast_to_int");

  if (_lhs_type == STRING_TYPE && _rhs_type != STRING_TYPE)
    _rhs = nullptr;
  return _rhs;
}

/**
 * Construct: Method
 * Name: NAssignment::code_generate
 * Desc: This may only be used after an `NVariableDeclaration` so first we
 *   check if the identifier exists within the current context and throw a
 *   CodeGenException if not. Using the global IR builder (see `BUILDER`) a
 *   store instruction is created linking the expression of `rhs` to the
 *   identifier of `lhs`
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NAssignment::code_generate(CodeGenContext &ctx) {
  ValTypeTuple _lhs_tuple = ctx.get_local(lhs.val);
  llvm::Value *_lhs = std::get<llvm::Value *>(_lhs_tuple);
  if (!_lhs)
    throw CodeGenException("Variable " + lhs.val +
                           " not defined in current block");
  llvm::Value *_rhs = cast_relevantly(rhs.code_generate(ctx), _lhs_tuple);
  if (_rhs)
    return BUILDER.CreateStore(_rhs, _lhs);
  return nullptr;
}

/**
 * Construct: Method
 * Name: NWrite::code_generate
 * Desc: Create the type-specific call to `printf` from libc, this obviously
 *   requires linking to libc to run, however the llvm::Module can be ran in
 *   the execution engine within the program without explicit linking - the
 *   LLVM IR output (.ll) can also be ran with LLI without explicit linking to
 *   libc
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NWrite::code_generate(CodeGenContext &ctx) {
  llvm::Value *_exp = exp.code_generate(ctx);
  llvm::Type *_exp_type = _exp->getType();
  llvm::SmallVector<llvm::Value *, 2> printf_args;
  if (_exp_type == DOUBLE_TYPE || _exp_type == INTEGER_TYPE)
    printf_args.push_back(ctx.fmt_specifiers.at("numeric"));
  else if (_exp_type == STRING_TYPE)
    printf_args.push_back(ctx.fmt_specifiers.at("string"));
  else
    throw CodeGenException("Write not yet implemented");
  printf_args.push_back(_exp);
  return BUILDER.CreateCall(ctx.printf_function, printf_args, "_printf_call");
}

/**
 * Construct: Method
 * Name: NRead::code_generate
 * Desc: See notes
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Not yet implemented
 */
llvm::Value *NRead::code_generate(CodeGenContext &ctx) {
  throw CodeGenException("Read not yet implemented");
}

/**
 * Construct: Method
 * Name: NReturnStatement::code_generate
 * Desc: Uses the global IR builder (see `BUILDER`) to create a return
 *   instruction (which returns from a function in a block). Multiple exit
 *   points can be specified in a functionn however the LLVM module
 *   verification will complain about this, e.g:
 *   """
 *     Terminator found in the middle of a basic block!
 *     label %xxxx
 *   """
 *   However, this thankfully does not break the compilation.
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NReturnStatement::code_generate(CodeGenContext &ctx) {
  return BUILDER.CreateRet(exp.code_generate(ctx));
}

/**
 * Construct: Method
 * Name: NExpressionStatement::code_generate
 * Desc: Uses the global IR builder (see `BUILDER`) to create a return
 *   instruction (which returns from a function in a block). Multiple exit
 *   points can be specified in a functionn however the LLVM module
 *   verification will complain about this, e.g:
 *   """
 *     Terminator found in the middle of a basic block!
 *     label %xxxx
 *   """
 *   However, this thankfully does not break the compilation.
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NExpressionStatement::code_generate(CodeGenContext &ctx) {
  return exp.code_generate(ctx);
}

/**
 * Construct: Function
 * Name: zero_value_for
 * Desc: Returns a pointer to the zero value (or equivalent) for the type
 *   passed to it, so:
 *     integer -> 0
 *     float -> 0.0
 *     string -> ""
 * Args:
 *   - type: The LLVM type for which a zero initializer should be created
 */
static llvm::Value *zero_value_for(llvm::Type *type) {
  if (type == DOUBLE_TYPE)
    return llvm::ConstantFP::get(DOUBLE_TYPE, 0.0);
  if (type == INTEGER_TYPE)
    return llvm::ConstantInt::get(INTEGER_TYPE, 0, true);
  if (type == STRING_TYPE)
    return get_i8_str_ptr("", "str_init_val");
  throw CodeGenException("Unknown variable type");
}

/**
 * Construct: Method
 * Name: NVariableDeclaration::code_generate
 * Desc: Allocates space for a variable of the relevant type under the name of
 *   the identifier in question. If the RHS is not null, an assignment (store
 *   instruction) is used to initialize the variable, if the RHS value is null,
 *   a zero value initializer is used
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NVariableDeclaration::code_generate(CodeGenContext &ctx) {
  llvm::Type *_lhs_type = type_of(type);
  llvm::Value *_lhs = BUILDER.CreateAlloca(_lhs_type, 0, lhs.val.c_str());
  ctx.set_local(lhs.val, _lhs, _lhs_type);
  if (rhs) {
    BUILDER.CreateStore(rhs->code_generate(ctx), _lhs);
  } else { // zero-initialize
    BUILDER.CreateStore(zero_value_for(_lhs_type), _lhs);
  }
  return _lhs;
}

/**
 * Construct: Method
 * Name: NFunctionDeclaration::code_generate
 * Desc: Creates a function under the identifier's name (`id`), create
 *   variable declarations and optional initilizers for each of the function's
 *   arguments, and generate the code for the function's block
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NFunctionDeclaration::code_generate(CodeGenContext &ctx) {

  llvm::BasicBlock *_current_block = BUILDER.GetInsertBlock();

  std::vector<llvm::Type *> arg_types;
  NVariableList::const_iterator it;

  /**
   * Create a vector of the function's argument's types for the function's
   * prototype
   */
  for (it = args.begin(); it != args.end(); it++)
    arg_types.push_back(type_of((*it)->type));

  /**
   * Create the function prototype with the arguments above and the specified
   * return type
   */
  llvm::FunctionType *_fn_type = llvm::FunctionType::get(
      type_of(type), llvm::makeArrayRef(arg_types), false);

  llvm::Function *_fn = llvm::Function::Create(
      _fn_type, llvm::GlobalValue::InternalLinkage, id.val.c_str(), ctx.module);

  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, id.val + "__entry", _fn, 0);

  /** Put the new block on the CodeGenBlock stack */
  ctx.push_block(_block);

  BUILDER.SetInsertPoint(_block);

  llvm::Function::arg_iterator arg_it = _fn->arg_begin();

  /** Create the arguments for the function (not just the types this time) */
  for (it = args.begin(); it != args.end(); it++) {
    // llvm::Value *arg_val = (*it)->code_generate(ctx);
    llvm::Value *_arg_value = arg_it++;
    _arg_value->setName((*it)->lhs.val.c_str());
    llvm::Value *_in_f_arg =
        BUILDER.CreateAlloca(type_of((*it)->type), 0, (*it)->lhs.val);
    BUILDER.CreateStore(_arg_value, _in_f_arg);
    ctx.set_local((*it)->lhs.val, _in_f_arg, type_of((*it)->type));
  }

  block.code_generate(ctx);

  /** After generating the code, pop the CodeGenBlock */
  ctx.pop_block();

  BUILDER.SetInsertPoint(_current_block);

  return _fn;
}

/**
 * Construct: Method
 * Name: NFunctionCall::code_generate
 * Desc: Create a call to an existing function using the global LLVM IR
 *   builder (see `BUILDER`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NFunctionCall::code_generate(CodeGenContext &ctx) {
  llvm::Function *fn = ctx.module->getFunction(id.val.c_str());

  if (!fn)
    throw CodeGenException("Attempted call on unknown function");

  std::vector<llvm::Value *> _args;

  NExpressionList::const_iterator it;
  for (it = args.begin(); it != args.end(); it++)
    _args.push_back((*it)->code_generate(ctx));

  return BUILDER.CreateCall(fn, _args, "_f_call");
}

/* ------ constructs ------ */

/**
 * Construct: Method
 * Name: NIfStatement::code_generate
 * Desc: Creates a compare of some sort and then a conditional branch. If an
 *   "else" (`els`) statement is present, including "else if"s, then generate
 *   the IR for those too.
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Multiple "else if" blocks will essentially call this function
 *     recursively managing the insert/current blocks each time
 */
llvm::Value *NIfStatement::code_generate(CodeGenContext &ctx) {

  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    throw CodeGenException("Invalid condition for `if`");
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

/**
 * Construct: Method
 * Name: NElseStatement::code_generate
 * Desc: The "else" is simply the last block before returning to the parent
 *   block, therefore we just call the NBlock::code_generate method
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NElseStatement::code_generate(CodeGenContext &ctx) {
  return block.code_generate(ctx);
}

/**
 * Construct: Method
 * Name: NWhileStatement::code_generate
 * Desc: Splits to a new block (the conditional block) and branches
 *   conditionally on the truthfulness of the condition (`cond`) to either
 *   the statement's block or to the continuation of the parent block
 * Args:
 *   - ctx: The CodeGenContext instance
 */
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
    throw CodeGenException("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(false), "while_cond");
  BUILDER.CreateCondBr(_cond, _block, _after);

  BUILDER.SetInsertPoint(_block);
  if (!block.code_generate(ctx))
    throw CodeGenException("Invalid while block");
  BUILDER.CreateBr(_while);

  BUILDER.SetInsertPoint(_after);

  return nullptr;
}

/**
 * Construct: Method
 * Name: NUntilStatement::code_generate
 * Desc: Splits to a new block (the conditional block) and branches
 *   conditionally on the falsity of the condition (`cond`) to either the
 *   statement's block or to the continuation of the parent block
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NUntilStatement::code_generate(CodeGenContext &ctx) {
  llvm::Function *_fn = BUILDER.GetInsertBlock()->getParent();
  llvm::BasicBlock *_until =
      llvm::BasicBlock::Create(LLVM_CTX, "until_cond", _fn);
  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, "until_block", _fn);
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, "until_aftr", _fn);

  BUILDER.CreateBr(_until);
  BUILDER.SetInsertPoint(_until);
  llvm::Value *_cond = cond.code_generate(ctx);
  if (!_cond)
    throw CodeGenException("Invalid condition");
  _cond = BUILDER.CreateICmpNE(_cond, BUILDER.getInt1(true), "until_cond");
  BUILDER.CreateCondBr(_cond, _block, _after);

  BUILDER.SetInsertPoint(_block);
  if (!block.code_generate(ctx))
    throw CodeGenException("Invalid until block");
  BUILDER.CreateBr(_until);

  BUILDER.SetInsertPoint(_after);

  return nullptr;
}
