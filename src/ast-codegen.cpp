#include "ast.hpp"
#include "codegen.hpp"

/** Returns an LLVM type based on the identifier */
static llvm::Type *type_of(const NIdentifier &type) {
  if (type.val == "integer")
    return INTEGER_TYPE;
  if (type.val == "float")
    return DOUBLE_TYPE;
  if (type.val == "string")
    return STRING_TYPE;
  if (type.val == "void")
    return VOID_TYPE;
  throw CodeGenException("Unknown type `" + type.val + "`");
}

/* -------- Type helpers -------- */

/**
 * Name: type_name
 * Construct: Function
 * Desc: The Sood name of an LLVM type, for use in error messages
 * Args:
 *   - type: The LLVM type
 */
std::string type_name(llvm::Type *type) {
  if (type == BOOLEAN_TYPE)
    return "boolean";
  if (type == INTEGER_TYPE)
    return "integer";
  if (type == DOUBLE_TYPE)
    return "float";
  if (type == STRING_TYPE)
    return "string";
  if (type == VOID_TYPE)
    return "void";
  std::string name;
  llvm::raw_string_ostream ost(name);
  type->print(ost);
  return name;
}

/**
 * Name: cast_to
 * Construct: Function
 * Desc: Converts a value to the given type, booleans may be used as integers
 *   or floats, and integers and floats may be converted between each other.
 *   Anything else is an error
 * Args:
 *   - val: The value to convert
 *   - type: The type to which the value should be converted
 *   - what: Description of where the conversion happens, for error messages
 */
llvm::Value *cast_to(llvm::Value *val, llvm::Type *type,
                     const std::string &what) {
  llvm::Type *from = val->getType();
  if (from == type)
    return val;
  if (from == BOOLEAN_TYPE && type == INTEGER_TYPE)
    return BUILDER.CreateZExt(val, INTEGER_TYPE, "bool_to_int");
  if (from == BOOLEAN_TYPE && type == DOUBLE_TYPE)
    return BUILDER.CreateUIToFP(val, DOUBLE_TYPE, "bool_to_float");
  if (from == INTEGER_TYPE && type == DOUBLE_TYPE)
    return BUILDER.CreateSIToFP(val, DOUBLE_TYPE, "int_to_float");
  if (from == DOUBLE_TYPE && type == INTEGER_TYPE)
    return BUILDER.CreateFPToSI(val, INTEGER_TYPE, "float_to_int");
  throw CodeGenException("Cannot use a " + type_name(from) + " as a " +
                         type_name(type) + " in " + what);
}

/**
 * Name: to_bool
 * Construct: Function
 * Desc: Converts a value to a boolean (`i1`) for use as a condition, numbers
 *   are true when non-zero
 * Args:
 *   - val: The value to convert
 *   - what: Description of where the conversion happens, for error messages
 */
static llvm::Value *to_bool(llvm::Value *val, const std::string &what) {
  llvm::Type *type = val->getType();
  if (type == BOOLEAN_TYPE)
    return val;
  if (type == INTEGER_TYPE)
    return BUILDER.CreateICmpNE(val, llvm::ConstantInt::get(INTEGER_TYPE, 0),
                                "to_bool");
  if (type == DOUBLE_TYPE)
    return BUILDER.CreateFCmpUNE(val, llvm::ConstantFP::get(DOUBLE_TYPE, 0.0),
                                 "to_bool");
  throw CodeGenException("Cannot use a " + type_name(type) +
                         " as a condition in " + what);
}

/**
 * Name: zero_value_for
 * Construct: Function
 * Desc: Returns the zero value (or equivalent) for the type passed to it, so:
 *     integer -> 0
 *     float -> 0.0
 *     string -> ""
 * Args:
 *   - ctx: The CodeGenContext instance
 *   - type: The LLVM type for which a zero initializer should be created
 */
llvm::Constant *zero_value_for(CodeGenContext &ctx, llvm::Type *type) {
  if (type == DOUBLE_TYPE)
    return llvm::ConstantFP::get(DOUBLE_TYPE, 0.0);
  if (type == INTEGER_TYPE)
    return llvm::ConstantInt::get(INTEGER_TYPE, 0, true);
  if (type == STRING_TYPE)
    return ctx.string_constant("");
  throw CodeGenException("No zero value for type " + type_name(type));
}

/* -------- Types  -------- */

/**
 * Name: NInteger::code_generate
 * Construct: Method
 * Desc: Create a constant of integer type (see `INTEGER_TYPE`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NInteger::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantInt::get(INTEGER_TYPE, val, true);
}

/**
 * Name: NFloat::code_generate
 * Construct: Method
 * Desc: Create a constant of floating type (see `DOUBLE_TYPE`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NFloat::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantFP::get(DOUBLE_TYPE, val);
}

/**
 * Name: NBoolean::code_generate
 * Construct: Method
 * Desc: Create a constant of boolean type (see `BOOLEAN_TYPE`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NBoolean::code_generate(CodeGenContext &ctx) {
  return llvm::ConstantInt::getBool(LLVM_CTX, val);
}

/**
 * Name: process_escape_chars
 * Construct: Function
 * Desc: The lexer reads strings verbatim, so any escaped characters are seen
 *   as two characters (`\\n` instead of `\n`, for example), this converts the
 *   common escape sequences into their single character
 * Args:
 *   - input: The string as written in the source code
 * Notes:
 *   - `\\` is converted to a single backslash, so `\\n` comes out as `\n`
 *     literally; unknown escapes are left as they are
 */
static std::string process_escape_chars(const std::string &input) {
  std::string out;
  out.reserve(input.size());
  for (size_t i = 0; i < input.size(); i++) {
    if (input[i] != '\\' || i + 1 == input.size()) {
      out += input[i];
      continue;
    }
    switch (input[++i]) {
    case 'n':
      out += '\n';
      break;
    case 'r':
      out += '\r';
      break;
    case 't':
      out += '\t';
      break;
    case '\\':
      out += '\\';
      break;
    default:
      out += '\\';
      out += input[i];
    }
  }
  return out;
}

/**
 * Name: NString::code_generate
 * Construct: Method
 * Desc: Returns a pointer to a global string with the contents of the `val`
 *   member
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NString::code_generate(CodeGenContext &ctx) {
  return ctx.string_constant(process_escape_chars(val));
}

/**
 * Name: NIdentifier::code_generate
 * Construct: Method
 * Desc: This is a reference to a variable and not a variable declaration, so
 *   first we must check to see if the identifier exists in the current
 *   context, if not, an exception is thrown. If the identifier does exist,
 *   load its value
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NIdentifier::code_generate(CodeGenContext &ctx) {
  ValTypeTuple var;
  if (!ctx.find_local(val, var))
    throw CodeGenException("Variable `" + val + "` is not defined");
  return BUILDER.CreateLoad(std::get<llvm::Type *>(var),
                            std::get<llvm::Value *>(var), val);
}

/* ----- Operative expressions ------ */

/**
 * Name: NUnaryExpression::code_generate
 * Construct: Method
 * Desc: Creates a unary operation applied to the relevant RHS using the
 *   global LLVM IR builder (see `BUILDER`)
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NUnaryExpression::code_generate(CodeGenContext &ctx) {
  llvm::Value *_rhs = rhs.code_generate(ctx);

  switch (op) {
  case OP_NOT:
    return BUILDER.CreateNot(to_bool(_rhs, "`not`"), "not");
  case OP_NEGATIVE:
    if (_rhs->getType() == DOUBLE_TYPE)
      return BUILDER.CreateFNeg(_rhs, "neg");
    return BUILDER.CreateNeg(cast_to(_rhs, INTEGER_TYPE, "`negative`"), "neg");
  default:
    throw CodeGenException("Invalid unary operator");
  }
}

/**
 * Name: logical_operation
 * Construct: Function
 * Desc: Generates a short-circuiting `and` or `alternatively`, the RHS is
 *   only evaluated if the LHS doesn't already decide the result
 * Args:
 *   - ctx: The CodeGenContext instance
 *   - op: Either `OP_AND` or `OP_ALTERNATIVELY`
 *   - lhs: The left hand side of the operation
 *   - rhs: The right hand side of the operation
 */
static llvm::Value *logical_operation(CodeGenContext &ctx, int op,
                                      NExpression &lhs, NExpression &rhs) {
  bool is_and = op == OP_AND;
  std::string what = is_and ? "`and`" : "`alternatively`";
  llvm::Function *_fn = ctx.current_function();

  llvm::Value *_lhs = to_bool(lhs.code_generate(ctx), what);
  llvm::BasicBlock *_lhs_end = BUILDER.GetInsertBlock();
  llvm::BasicBlock *_rhs_block =
      llvm::BasicBlock::Create(LLVM_CTX, is_and ? "and_rhs" : "alt_rhs", _fn);
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, is_and ? "and_cnt" : "alt_cnt");

  if (is_and)
    BUILDER.CreateCondBr(_lhs, _rhs_block, _after);
  else
    BUILDER.CreateCondBr(_lhs, _after, _rhs_block);

  BUILDER.SetInsertPoint(_rhs_block);
  llvm::Value *_rhs = to_bool(rhs.code_generate(ctx), what);
  llvm::BasicBlock *_rhs_end = BUILDER.GetInsertBlock();
  BUILDER.CreateBr(_after);

  _fn->insert(_fn->end(), _after);
  BUILDER.SetInsertPoint(_after);
  llvm::PHINode *_phi = BUILDER.CreatePHI(BOOLEAN_TYPE, 2, is_and ? "and" : "alt");
  _phi->addIncoming(BUILDER.getInt1(!is_and), _lhs_end);
  _phi->addIncoming(_rhs, _rhs_end);
  return _phi;
}

/**
 * Name: comparison_predicates
 * Construct: Function
 * Desc: Maps a comparison operator to its integer and floating point LLVM
 *   predicates, returns false if the operator is not a comparison
 */
static bool comparison_predicates(int op, llvm::CmpInst::Predicate &i_pred,
                                  llvm::CmpInst::Predicate &f_pred) {
  switch (op) {
  case OP_EQUAL_TO:
    i_pred = llvm::CmpInst::ICMP_EQ, f_pred = llvm::CmpInst::FCMP_OEQ;
    return true;
  case OP_NOT_EQUAL_TO:
    i_pred = llvm::CmpInst::ICMP_NE, f_pred = llvm::CmpInst::FCMP_UNE;
    return true;
  case OP_LESS_THAN:
    i_pred = llvm::CmpInst::ICMP_SLT, f_pred = llvm::CmpInst::FCMP_OLT;
    return true;
  case OP_LESS_THAN_EQUAL_TO:
    i_pred = llvm::CmpInst::ICMP_SLE, f_pred = llvm::CmpInst::FCMP_OLE;
    return true;
  case OP_MORE_THAN:
    i_pred = llvm::CmpInst::ICMP_SGT, f_pred = llvm::CmpInst::FCMP_OGT;
    return true;
  case OP_MORE_THAN_EQUAL_TO:
    i_pred = llvm::CmpInst::ICMP_SGE, f_pred = llvm::CmpInst::FCMP_OGE;
    return true;
  default:
    return false;
  }
}

/**
 * Name: NBinaryExpression::code_generate
 * Construct: Method
 * Desc: Creates a binary operation between the relevant LHS and RHS. Booleans
 *   are treated as integers, and if either side is a float, both sides are
 *   converted to floats. Strings may be compared (with libc's `strcmp`) but
 *   not used in arithmetic
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NBinaryExpression::code_generate(CodeGenContext &ctx) {
  if (op == OP_AND || op == OP_ALTERNATIVELY)
    return logical_operation(ctx, op, lhs, rhs);

  llvm::Value *_lhs = lhs.code_generate(ctx);
  llvm::Value *_rhs = rhs.code_generate(ctx);

  llvm::CmpInst::Predicate i_pred, f_pred;
  bool is_comparison = comparison_predicates(op, i_pred, f_pred);

  bool lhs_is_str = _lhs->getType() == STRING_TYPE;
  bool rhs_is_str = _rhs->getType() == STRING_TYPE;
  if (lhs_is_str || rhs_is_str) {
    if (!(lhs_is_str && rhs_is_str))
      throw CodeGenException("Cannot use a string and a " +
                             type_name(lhs_is_str ? _rhs->getType()
                                                  : _lhs->getType()) +
                             " in the same operation");
    if (!is_comparison)
      throw CodeGenException("Arithmetic is not supported on strings");
    llvm::Value *_cmp = BUILDER.CreateCall(ctx.get_libc("strcmp"),
                                           {_lhs, _rhs}, "strcmp");
    return BUILDER.CreateICmp(i_pred, _cmp, BUILDER.getInt32(0), "str_cmp");
  }

  bool is_float =
      _lhs->getType() == DOUBLE_TYPE || _rhs->getType() == DOUBLE_TYPE;
  llvm::Type *_type = is_float ? DOUBLE_TYPE : INTEGER_TYPE;
  _lhs = cast_to(_lhs, _type, "binary operation");
  _rhs = cast_to(_rhs, _type, "binary operation");

  if (is_comparison)
    return BUILDER.CreateCmp(is_float ? f_pred : i_pred, _lhs, _rhs, "cmp");

  switch (op) {
  case OP_PLUS:
    return is_float ? BUILDER.CreateFAdd(_lhs, _rhs, "add")
                    : BUILDER.CreateAdd(_lhs, _rhs, "add");
  case OP_MINUS:
    return is_float ? BUILDER.CreateFSub(_lhs, _rhs, "sub")
                    : BUILDER.CreateSub(_lhs, _rhs, "sub");
  case OP_MULTIPLIED_BY:
    return is_float ? BUILDER.CreateFMul(_lhs, _rhs, "mul")
                    : BUILDER.CreateMul(_lhs, _rhs, "mul");
  case OP_DIVIDED_BY:
    return is_float ? BUILDER.CreateFDiv(_lhs, _rhs, "div")
                    : BUILDER.CreateSDiv(_lhs, _rhs, "div");
  case OP_MODULO:
    return is_float ? BUILDER.CreateFRem(_lhs, _rhs, "mod")
                    : BUILDER.CreateSRem(_lhs, _rhs, "mod");
  default:
    throw CodeGenException("Invalid binary operator");
  }
}

/* ------ Chunks ------*/

/**
 * Name: NBlock::code_generate
 * Construct: Method
 * Desc: Iterates over the statements of the block (see `NBlock` : `stmts`)
 *   and calls their respective `code_generate` methods
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Statements following a `return` can never be reached and LLVM does not
 *     allow instructions after a terminator, so they are not generated
 */
llvm::Value *NBlock::code_generate(CodeGenContext &ctx) {
  llvm::Value *last = nullptr;
  for (NStatement *stmt : stmts) {
    if (BUILDER.GetInsertBlock()->hasTerminator())
      break;
    try {
      last = stmt->code_generate(ctx);
    } catch (CodeGenException &e) {
      if (!e.line)
        e.line = stmt->line;
      throw;
    }
  }
  return last;
}

/**
 * Name: NAssignment::code_generate
 * Construct: Method
 * Desc: This may only be used after an `NVariableDeclaration` so first we
 *   check if the identifier exists within the current context and throw a
 *   CodeGenException if not. A store instruction then writes the value of
 *   `rhs` (converted to the variable's type) to the variable
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NAssignment::code_generate(CodeGenContext &ctx) {
  ValTypeTuple var;
  if (!ctx.find_local(lhs.val, var))
    throw CodeGenException("Variable `" + lhs.val + "` is not defined");
  llvm::Value *_rhs = cast_to(rhs.code_generate(ctx), std::get<llvm::Type *>(var),
                              "assignment to `" + lhs.val + "`");
  return BUILDER.CreateStore(_rhs, std::get<llvm::Value *>(var));
}

/**
 * Name: stream_fd
 * Construct: Function
 * Desc: The file descriptor for the named stream of a `read` or `write`
 * Args:
 *   - exp: The expression naming the stream, must be an identifier
 *   - names: The accepted stream names, in file descriptor order from `first`
 *   - first: The file descriptor of the first name
 */
static int stream_fd(NExpression &exp, std::vector<std::string> names,
                     int first) {
  NIdentifier *ident = dynamic_cast<NIdentifier *>(&exp);
  if (ident)
    for (size_t i = 0; i < names.size(); i++)
      if (ident->val == names[i])
        return first + i;
  std::string msg = "Expected ";
  for (size_t i = 0; i < names.size(); i++)
    msg += (i ? " or `" : "`") + names[i] + "`";
  throw CodeGenException(msg);
}

/**
 * Name: NWrite::code_generate
 * Construct: Method
 * Desc: Create the type-specific call to `dprintf` from libc, writing to
 *   either `stdout` or `stderr`. Booleans are written as `true` or `false`
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - `dprintf` is unbuffered, so writes to `stdout` and `stderr` are never
 *     reordered
 */
llvm::Value *NWrite::code_generate(CodeGenContext &ctx) {
  int fd = stream_fd(to, {"stdout", "stderr"}, 1);
  llvm::Value *_exp = exp.code_generate(ctx);
  llvm::Type *_exp_type = _exp->getType();
  llvm::Value *_fmt;
  if (_exp_type == INTEGER_TYPE) {
    _fmt = ctx.string_constant("%lld");
  } else if (_exp_type == DOUBLE_TYPE) {
    _fmt = ctx.string_constant("%g");
  } else if (_exp_type == STRING_TYPE) {
    _fmt = ctx.string_constant("%s");
  } else if (_exp_type == BOOLEAN_TYPE) {
    _fmt = ctx.string_constant("%s");
    _exp = BUILDER.CreateSelect(_exp, ctx.string_constant("true"),
                                ctx.string_constant("false"), "bool_str");
  } else {
    throw CodeGenException("Cannot write a " + type_name(_exp_type));
  }
  return BUILDER.CreateCall(ctx.get_libc("dprintf"),
                            {BUILDER.getInt32(fd), _fmt, _exp}, "write");
}

/**
 * Name: NRead::code_generate
 * Construct: Method
 * Desc: Reads a value from `stdin` into a variable using libc's `scanf`,
 *   integers and floats are read as numbers, strings read the rest of the line
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Strings are read into a new 1KiB buffer which is never freed
 *   - If the input cannot be parsed, the variable is left unchanged (strings
 *     become empty)
 */
llvm::Value *NRead::code_generate(CodeGenContext &ctx) {
  stream_fd(from, {"stdin"}, 0);
  NIdentifier *ident = dynamic_cast<NIdentifier *>(&to);
  ValTypeTuple var;
  if (!ident)
    throw CodeGenException("Can only read into a variable");
  if (!ctx.find_local(ident->val, var))
    throw CodeGenException("Variable `" + ident->val + "` is not defined");

  llvm::Value *_ptr = std::get<llvm::Value *>(var);
  llvm::Type *_type = std::get<llvm::Type *>(var);
  llvm::Function *scanf = ctx.get_libc("scanf");

  if (_type == INTEGER_TYPE)
    return BUILDER.CreateCall(scanf, {ctx.string_constant(" %lld"), _ptr},
                              "read");
  if (_type == DOUBLE_TYPE)
    return BUILDER.CreateCall(scanf, {ctx.string_constant(" %lf"), _ptr},
                              "read");

  const int buf_size = 1024;
  llvm::Value *_buf =
      BUILDER.CreateCall(ctx.get_libc("malloc"),
                         {llvm::ConstantInt::get(INTEGER_TYPE, buf_size)}, "buf");
  BUILDER.CreateStore(BUILDER.getInt8(0), _buf);
  std::string fmt = " %" + std::to_string(buf_size - 1) + "[^\n]";
  llvm::Value *_read = BUILDER.CreateCall(
      scanf, {ctx.string_constant(fmt), _buf}, "read");
  BUILDER.CreateStore(_buf, _ptr);
  return _read;
}

/**
 * Name: NReturnStatement::code_generate
 * Construct: Method
 * Desc: Creates a return instruction, converting the value to the function's
 *   return type. A `return` at the top level of the program exits with that
 *   integer as the status code
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NReturnStatement::code_generate(CodeGenContext &ctx) {
  llvm::Type *_ret_type = ctx.current_function()->getReturnType();
  if (_ret_type == VOID_TYPE)
    throw CodeGenException("Cannot return a value from a void function");
  llvm::Value *_exp = exp.code_generate(ctx);
  if (_ret_type == BUILDER.getInt32Ty()) { // `main`
    _exp = cast_to(_exp, INTEGER_TYPE, "exit status");
    return BUILDER.CreateRet(BUILDER.CreateTrunc(_exp, _ret_type));
  }
  return BUILDER.CreateRet(cast_to(_exp, _ret_type, "return value"));
}

/**
 * Name: NExpressionStatement::code_generate
 * Construct: Method
 * Desc: Generate the expression, discarding its value
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NExpressionStatement::code_generate(CodeGenContext &ctx) {
  return exp.code_generate(ctx);
}

/**
 * Name: NVariableDeclaration::code_generate
 * Construct: Method
 * Desc: Allocates space for a variable of the relevant type under the name of
 *   the identifier in question. If the RHS is not null, it is used to
 *   initialize the variable, otherwise a zero value initializer is used
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - The RHS is generated before the variable is declared, so it may refer to
 *     a variable of the same name in an outer scope
 */
llvm::Value *NVariableDeclaration::code_generate(CodeGenContext &ctx) {
  llvm::Type *_type = type_of(type);
  if (_type == VOID_TYPE)
    throw CodeGenException("Variable `" + lhs.val + "` cannot be void");

  llvm::Value *_init = nullptr;
  if (rhs)
    _init = cast_to(rhs->code_generate(ctx), _type,
                    "initialization of `" + lhs.val + "`");

  bool is_global = ctx.in_global_scope();
  llvm::Value *_var = ctx.create_variable(lhs.val, _type);

  /* Globals are zero-initialized already, locals are re-zeroed on each
   * declaration as the declaration may be within a loop */
  if (!_init && !is_global)
    _init = zero_value_for(ctx, _type);
  if (_init)
    BUILDER.CreateStore(_init, _var);
  return _var;
}

/**
 * Name: NFunctionDeclaration::declare
 * Construct: Method
 * Desc: Creates the function's prototype in the module, this is done for all
 *   top-level functions before any other code is generated so that functions
 *   may be called before (in the source code) they are declared
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Function *NFunctionDeclaration::declare(CodeGenContext &ctx) {
  if (llvm_fn)
    return llvm_fn;
  if (ctx.fn_decls.count(id.val))
    throw CodeGenException("Function `" + id.val + "` is already declared");
  if (ctx.is_reserved(id.val))
    throw CodeGenException("`" + id.val + "` is a reserved function name");

  std::vector<llvm::Type *> arg_types;
  bool has_default = false;
  for (NVariableDeclaration *arg : args) {
    llvm::Type *_type = type_of(arg->type);
    if (_type == VOID_TYPE)
      throw CodeGenException("Argument `" + arg->lhs.val + "` cannot be void");
    if (has_default && !arg->rhs)
      throw CodeGenException("Argument `" + arg->lhs.val +
                             "` must have a default value as it follows one "
                             "which does");
    has_default |= arg->rhs != nullptr;
    arg_types.push_back(_type);
  }

  llvm::FunctionType *_fn_type =
      llvm::FunctionType::get(type_of(type), arg_types, false);
  llvm_fn = llvm::Function::Create(
      _fn_type, llvm::GlobalValue::InternalLinkage, id.val, ctx.module.get());

  auto arg_it = llvm_fn->arg_begin();
  for (NVariableDeclaration *arg : args)
    (arg_it++)->setName(arg->lhs.val);

  ctx.fn_decls[id.val] = this;
  return llvm_fn;
}

/**
 * Name: NFunctionDeclaration::code_generate
 * Construct: Method
 * Desc: Generates the function's body: each argument is copied into a local
 *   variable (so it may be assigned to) and then the function's block is
 *   generated in a new scope
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - A function which doesn't end in a `return` returns the zero value of
 *     its return type
 */
llvm::Value *NFunctionDeclaration::code_generate(CodeGenContext &ctx) {
  llvm::Function *_fn = declare(ctx);

  /** Restores the insert point of the enclosing code when we're done */
  llvm::IRBuilderBase::InsertPointGuard guard(BUILDER);

  BUILDER.SetInsertPoint(
      llvm::BasicBlock::Create(LLVM_CTX, id.val + "__entry", _fn));
  ctx.push_scope(true);

  auto arg_it = _fn->arg_begin();
  for (NVariableDeclaration *arg : args) {
    llvm::Value *_arg_value = arg_it++;
    llvm::Value *_var = ctx.create_variable(arg->lhs.val, _arg_value->getType());
    BUILDER.CreateStore(_arg_value, _var);
  }

  block.code_generate(ctx);

  if (!BUILDER.GetInsertBlock()->hasTerminator()) {
    if (_fn->getReturnType() == VOID_TYPE)
      BUILDER.CreateRetVoid();
    else
      BUILDER.CreateRet(zero_value_for(ctx, _fn->getReturnType()));
  }

  ctx.pop_scope();
  return _fn;
}

/**
 * Name: NFunctionCall::code_generate
 * Construct: Method
 * Desc: Create a call to a declared function, any arguments not given are
 *   filled in with their default values and all are converted to the types
 *   of the function's parameters
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NFunctionCall::code_generate(CodeGenContext &ctx) {
  auto decl_it = ctx.fn_decls.find(id.val);
  if (decl_it == ctx.fn_decls.end())
    throw CodeGenException("Call to unknown function `" + id.val + "`");
  NFunctionDeclaration *decl = decl_it->second;
  llvm::Function *_fn = decl->llvm_fn;

  if (args.size() > decl->args.size())
    throw CodeGenException("Too many arguments given to `" + id.val + "`");

  std::vector<llvm::Value *> _args;
  for (size_t i = 0; i < decl->args.size(); i++) {
    NVariableDeclaration *param = decl->args[i];
    NExpression *arg = i < args.size() ? args[i] : param->rhs;
    if (!arg)
      throw CodeGenException("Missing argument `" + param->lhs.val +
                             "` in call to `" + id.val + "`");
    _args.push_back(cast_to(arg->code_generate(ctx),
                            _fn->getArg(i)->getType(),
                            "argument `" + param->lhs.val + "` of `" +
                                id.val + "`"));
  }

  /** LLVM does not allow naming a void value */
  bool is_void = _fn->getReturnType() == VOID_TYPE;
  return BUILDER.CreateCall(_fn, _args, is_void ? "" : id.val + "_call");
}

/* ------ constructs ------ */

/**
 * Name: NIfStatement::code_generate
 * Construct: Method
 * Desc: Creates a conditional branch to the `then` block and either the
 *   `else` block (if `els` is present, including "else if"s) or the code
 *   following the `if`
 * Args:
 *   - ctx: The CodeGenContext instance
 * Notes:
 *   - Multiple "else if" blocks will essentially call this function
 *     recursively managing the insert/current blocks each time
 */
llvm::Value *NIfStatement::code_generate(CodeGenContext &ctx) {
  llvm::Value *_cond = to_bool(cond.code_generate(ctx), "`if`");
  llvm::Function *_fn = ctx.current_function();

  llvm::BasicBlock *_then = llvm::BasicBlock::Create(LLVM_CTX, "if_then", _fn);
  llvm::BasicBlock *_else =
      els ? llvm::BasicBlock::Create(LLVM_CTX, "if_else") : nullptr;
  llvm::BasicBlock *_aftr = llvm::BasicBlock::Create(LLVM_CTX, "if_cnt");
  BUILDER.CreateCondBr(_cond, _then, els ? _else : _aftr);

  BUILDER.SetInsertPoint(_then);
  ctx.push_scope();
  block.code_generate(ctx);
  ctx.pop_scope();
  ctx.branch_to(_aftr);

  if (els) {
    _fn->insert(_fn->end(), _else);
    BUILDER.SetInsertPoint(_else);
    els->code_generate(ctx);
    ctx.branch_to(_aftr);
  }

  _fn->insert(_fn->end(), _aftr);
  BUILDER.SetInsertPoint(_aftr);

  // NOTE: Nothing specific to return and handles block movement internally
  return nullptr;
}

/**
 * Name: NElseStatement::code_generate
 * Construct: Method
 * Desc: The "else" is simply the last block before returning to the parent
 *   block, therefore we just generate the block in a new scope
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NElseStatement::code_generate(CodeGenContext &ctx) {
  ctx.push_scope();
  llvm::Value *last = block.code_generate(ctx);
  ctx.pop_scope();
  return last;
}

/**
 * Name: loop
 * Construct: Function
 * Desc: Generates a loop which evaluates the condition, then branches either
 *   to the block (which branches back to the condition) or past the loop
 * Args:
 *   - ctx: The CodeGenContext instance
 *   - cond: The loop's condition
 *   - block: The loop's block of statements
 *   - name: The kind of loop, `while` or `until`
 *   - run_while: Whether the block runs while the condition is true (`while`)
 *     or while it is false (`until`)
 */
static void loop(CodeGenContext &ctx, NExpression &cond, NBlock &block,
                 const std::string &name, bool run_while) {
  llvm::Function *_fn = ctx.current_function();
  llvm::BasicBlock *_cond_block =
      llvm::BasicBlock::Create(LLVM_CTX, name + "_cond", _fn);
  llvm::BasicBlock *_block =
      llvm::BasicBlock::Create(LLVM_CTX, name + "_block", _fn);
  llvm::BasicBlock *_after =
      llvm::BasicBlock::Create(LLVM_CTX, name + "_aftr", _fn);

  BUILDER.CreateBr(_cond_block);
  BUILDER.SetInsertPoint(_cond_block);
  llvm::Value *_cond = to_bool(cond.code_generate(ctx), "`" + name + "`");
  if (run_while)
    BUILDER.CreateCondBr(_cond, _block, _after);
  else
    BUILDER.CreateCondBr(_cond, _after, _block);

  BUILDER.SetInsertPoint(_block);
  ctx.push_scope();
  block.code_generate(ctx);
  ctx.pop_scope();
  ctx.branch_to(_cond_block);

  /* The condition may have created blocks (e.g. for `and`), keep the block
   * following the loop last */
  _after->moveAfter(&_fn->back());
  BUILDER.SetInsertPoint(_after);
}

/**
 * Name: NWhileStatement::code_generate
 * Construct: Method
 * Desc: Runs the block while the condition (`cond`) is true
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NWhileStatement::code_generate(CodeGenContext &ctx) {
  loop(ctx, cond, block, "while", true);
  return nullptr;
}

/**
 * Name: NUntilStatement::code_generate
 * Construct: Method
 * Desc: Runs the block until the condition (`cond`) is true
 * Args:
 *   - ctx: The CodeGenContext instance
 */
llvm::Value *NUntilStatement::code_generate(CodeGenContext &ctx) {
  loop(ctx, cond, block, "until", false);
  return nullptr;
}
