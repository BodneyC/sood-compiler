#ifndef _AST_HPP_
#define _AST_HPP_

#include <cstdint>
#include <iostream>
#include <llvm/IR/Value.h>
#include <string>
#include <vector>

class NVariableDeclaration;
class NStatement;
class NExpression;
class CodeGenContext;

/**
 * Name: OPS
 * Construct: Enum
 * Desc: The list of binary and unary, arithmetic and boolean operations
 */
enum OPS {
  OP_EQUAL_TO,
  OP_NOT_EQUAL_TO,
  OP_LESS_THAN,
  OP_LESS_THAN_EQUAL_TO,
  OP_MORE_THAN,
  OP_MORE_THAN_EQUAL_TO,
  OP_NOT,
  OP_NEGATIVE,
  OP_PLUS,
  OP_MINUS,
  OP_MULTIPLIED_BY,
  OP_DIVIDED_BY,
  OP_MODULO,
  OP_ALSO,
  OP_ALTERNATIVELY,
};

typedef std::vector<NVariableDeclaration *> NVariableList;
typedef std::vector<NStatement *> NStatementList;
typedef std::vector<NExpression *> NExpressionList;

/* ------------- Base Nodes ------------- */

/**
 * Name: Node
 * Construct: Class
 * Desc: The generic base-node used in the formation of the AST, all other
 *   nodes are based on this one
 */
class Node {
protected:
  virtual void print(std::ostream &) const = 0;

public:
  virtual ~Node() {}
  virtual llvm::Value *code_generate(CodeGenContext &) = 0;
  friend std::ostream &operator<<(std::ostream &out, Node const &obj) {
    obj.print(out);
    return out;
  }
};

/**
 * Name: NExpression
 * Construct: Class
 * Desc: `Node` is split into two subclasses, this subclass is the parent of all
 *   expression nodes
 * Example: Identifiers, like `x`, are expressions
 */
class NExpression : public Node {};

/**
 * Name: NStatement
 * Construct: Class
 * Desc: `Node` is split into two subclasses, this subclass is the parent of all
 *   statement nodes
 * Example: Assignments, like `x = y`, are expressions
 */
class NStatement : public Node {};

/* -------- Based on NExpression -------- */

/**
 * Name: NInteger
 * Construct: Class
 * Desc: Simple integer value node
 * Members:
 *   - val: The 64-bit, signed integer value
 */
class NInteger : public NExpression {
public:
  std::int64_t val;
  NInteger(std::int64_t val) : val(val) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NFloat
 * Construct: Class
 * Desc: Simple floating point value node
 * Members:
 *   - val: The 64-bit floating point value (double)
 */
class NFloat : public NExpression {
public:
  double val;
  NFloat(double val) : val(val) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NString
 * Construct: Class
 * Desc: String value node, implemented with a C++ std::string
 * Members:
 *   - val: The C++ string value
 */
class NString : public NExpression {
public:
  std::string val;
  NString(std::string val) {
    // Cut the surrounding quotes, could be more dynamic...
    this->val = val.substr(1, val.size() - 2);
  }
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NIdentifier
 * Construct: Class
 * Desc: Identifier node, for example in the phrase `x = y`, both `x` and `y`
 *   would be represented in the AST by NIdentifiers
 * Members:
 *   - val: The name of the identifier
 */
class NIdentifier : public NExpression {
public:
  std::string val;
  NIdentifier() {}
  NIdentifier(std::string val) : val(val) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NFunctionCall
 * Construct: Class
 * Desc: Node containing any function call
 * Members:
 *   - id: The name of the function to be called
 *   - args: Vector of children of NExpression representing the arguments to
 *     be passed to the function
 */
class NFunctionCall : public NExpression {
public:
  NIdentifier &id;
  NExpressionList args;
  NFunctionCall(NIdentifier &id) : id(id) {}
  NFunctionCall(NIdentifier &id, NExpressionList args)
      : id(id), args(args) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NUnaryExpression
 * Construct: Class
 * Desc: Node containing a unary operation and the expression to which it is
 *   applied
 * Members:
 *   - op: The unary operation to be used (see `OPS`)
 *   - rhs: Child of NExpression to which the `op` should be applied
 */
class NUnaryExpression : public NExpression {
public:
  int op;
  NExpression &rhs;
  NUnaryExpression(int op, NExpression &rhs) : rhs(rhs), op(op) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NBinaryExpression
 * Construct: Class
 * Desc: Node containing the left and right expressions and the operation of a
 *   binary operation
 * Members:
 *   - op: The binary operation to be used (see `OPS`)
 *   - lhs: The left hand side of the binary operation
 *   - rhs: The right hand side of the binary operation
 */
class NBinaryExpression : public NExpression {
public:
  int op;
  NExpression &lhs;
  NExpression &rhs;
  NBinaryExpression(NExpression &lhs, int op, NExpression &rhs)
      : lhs(lhs), rhs(rhs), op(op) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NBlock
 * Construct: Class
 * Desc: An NExpression-based container of statments representing a "block" of
 *   code
 * Members:
 *   - stmts: Vector of children of NStatement comprising the statements of
 *     the block
 */
class NBlock : public NExpression {
public:
  NStatementList stmts;
  NBlock() {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NAssignment
 * Construct: Class
 * Desc: Node representing an assignment of an expression to an identifier
 * Members:
 *   - lhs: The identifier to be assigned a vlaue
 *   - rhs: The expression to assign to `lhs`
 */
class NAssignment : public NStatement {
public:
  NIdentifier &lhs;
  NExpression &rhs;
  NAssignment(NIdentifier &lhs, NExpression &rhs) : lhs(lhs), rhs(rhs) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/* -------- Based on NStatement --------- */

/**
 * Name: NRead
 * Construct: Class
 * Desc: Node for a `read` statement
 * Members:
 *   - from: The source from which to read data
 *   - rhs: The identifier into which the data should be read
 * notes:
 *   - Not yet implemented
 */
class NRead : public NStatement {
public:
  NExpression &from;
  NExpression &to;
  NRead(NExpression &from, NExpression &to) : from(from), to(to) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NWrite
 * Construct: Class
 * Desc: Node for a `write` statement
 * Members:
 *   - exp: The expression to write
 *   - to: The sink to which the data should be written
 * notes:
 *   - The `to` concept is not yet implemented, all `NWrite`s go to stdout
 */
class NWrite : public NStatement {
public:
  NExpression &exp;
  NExpression &to;
  NWrite(NExpression &exp, NExpression &to) : exp(exp), to(to) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NReturnStatement
 * Construct: Class
 * Desc: Node representing a return statement from a function
 * Members:
 *   - exp: The expression to return from the function
 */
class NReturnStatement : public NStatement {
public:
  NExpression &exp;
  NReturnStatement(NExpression &exp) : exp(exp) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NExpressionStatement
 * Construct: Class
 * Desc: Node representing an expression which can be used as a statment
 * Members:
 *   - exp - The expression to run as though it were a statement
 * Example: A function call without an assignment is only an expression, to be
 *   able to add it to a block, this must be interpreted as a statement. In
 *   this example, this function call would be added to an
 *   `NExpressionStatement`
 */
class NExpressionStatement : public NStatement {
public:
  NExpression &exp;
  NExpressionStatement(NExpression &exp) : exp(exp) {}
  virtual llvm::Value *code_generate(CodeGenContext &context);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NVariableDeclaration
 * Construct: Class
 * Desc: Node representing a variable declaration and, optionally, an
 *   assignment
 * Members:
 *   - type: The type of the variable to be declared
 *   - lhs: The identifier of the variable to be declared
 *   - rhs: The expression used in initialization, `nullptr` if no
 *     initialization is wanted
 */
class NVariableDeclaration : public NStatement {
public:
  const NIdentifier &type;
  NIdentifier &lhs;
  NExpression *rhs;
  NVariableDeclaration(NIdentifier &type, NIdentifier &lhs)
      : type(type), lhs(lhs) {
    rhs = nullptr;
  }
  NVariableDeclaration(NIdentifier &type, NIdentifier &lhs, NExpression *rhs)
      : type(type), lhs(lhs), rhs(rhs) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NUntilStatement
 * Construct: Class
 * Desc: Node representing an `until` statement, which is essentially the
 *   inverse of a `while` statement
 * Members:
 *   - cond: The condition under which the statements of `block` will be ran
 *   - block: The block of statement to run if the condition is met
 */
class NUntilStatement : public NStatement {
public:
  NExpression &cond;
  NBlock &block;
  NUntilStatement(NExpression &cond, NBlock &block)
      : cond(cond), block(block) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NWhileStatement
 * Construct: Class
 * Desc: Node representing a `while` statement, which is essentially the
 *   inverse of an `until` statement
 * Members:
 *   - cond: The condition under which the statements of `block` will be ran
 *   - block: The block of statement to run if the condition is met
 */
class NWhileStatement : public NStatement {
public:
  NExpression &cond;
  NBlock &block;
  NWhileStatement(NExpression &cond, NBlock &block)
      : cond(cond), block(block) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NElseStatement
 * Construct: Class
 * Desc: Node representing the "else" block of an `if` statement
 * Members:
 *   - block: The block of statements of the "else"
 */
class NElseStatement : public NStatement {
public:
  NBlock &block;
  NElseStatement(NBlock &block) : block(block) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NIfStatement
 * Construct: Class
 * Desc: Node representing an `if` statement
 * Members:
 *   - cond: The condition under which the statements of `block` will be ran
 *   - block: The block of statement to run if the condition is met
 *   - els: The `else` portion of the if, this commonly either another
 *     NIfStatement which can be done repeated to implement `else if` logic;
 *     or, an NElseStatement for the final `else`
 */
class NIfStatement : public NStatement {
public:
  NExpression &cond;
  NBlock &block;
  NStatement *els;
  NIfStatement(NExpression &cond, NBlock &block) : cond(cond), block(block) {
    els = nullptr;
  }
  NIfStatement(NExpression &cond, NBlock &block, NStatement *els)
      : cond(cond), block(block), els(els) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/**
 * Name: NFunctionDeclaration
 * Construct: Class
 * Desc: Node representing the declaration of a function and it's statements
 * Members:
 *   - id: The function identifier
 *   - args: Vector of NVariableDeclaration representing the variable
 *     arguments created when the function is called
 *   - block: The block of statements to be ran upon function call
 */
class NFunctionDeclaration : public NStatement {
public:
  const NIdentifier &type;
  NIdentifier &id;
  NVariableList args;
  NBlock &block;
  NFunctionDeclaration(NIdentifier &type, NIdentifier &id, NBlock &block)
      : type(type), id(id), block(block) {}
  NFunctionDeclaration(NIdentifier &type, NIdentifier &id, NVariableList args,
                       NBlock &block)
      : type(type), id(id), args(args), block(block) {}
  virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

#endif
