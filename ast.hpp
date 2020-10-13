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

typedef std::vector<NVariableDeclaration *> NVariableList;
typedef std::vector<NStatement *> NStatementList;
typedef std::vector<NExpression *> NExpressionList;

/* ------------- Base Nodes ------------- */

class Node {
protected:
  virtual void print(std::ostream &) const = 0;

public:
  virtual ~Node() {}
  // virtual llvm::Value *code_generate(CodeGenContext &) = 0;
  friend std::ostream &operator<<(std::ostream &out, Node const &obj) {
    obj.print(out);
    return out;
  }
};

class NExpression : public Node {};

class NStatement : public Node {};

/* -------- Based on NExpression -------- */

class NInteger : public NExpression {
public:
  std::int64_t val;
  NInteger(std::int64_t val) : val(val) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NFloat : public NExpression {
public:
  double val;
  NFloat(double val) : val(val) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NString : public NExpression {
public:
  std::string val;
  NString(std::string val) {
    // Cut the surrounding quotes, could be more dynamic...
    this->val = val.substr(1, val.size() - 2);
  }
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NIdentifier : public NExpression {
public:
  std::string val;
  NIdentifier() {}
  NIdentifier(std::string val) : val(val) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NFunctionCall : public NExpression {
public:
  NIdentifier &func;
  NExpressionList args;
  NFunctionCall(NIdentifier &func) : func(func) {}
  NFunctionCall(NIdentifier &func, NExpressionList args)
      : func(func), args(args) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NUnaryExpression : public NExpression {
public:
  int op;
  NExpression &rhs;
  NUnaryExpression(int op, NExpression &rhs) : rhs(rhs), op(op) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NBinaryExpression : public NExpression {
public:
  int op;
  NExpression &lhs;
  NExpression &rhs;
  NBinaryExpression(NExpression &lhs, int op, NExpression &rhs)
      : lhs(lhs), rhs(rhs), op(op) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NBlock : public NExpression {
public:
  NStatementList stmts;
  NBlock() {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

/* -------- Based on NStatement --------- */

class NAssignment : public NExpression {
public:
  NIdentifier &lhs;
  NExpression &rhs;
  NAssignment(NIdentifier &lhs, NExpression &rhs) : lhs(lhs), rhs(rhs) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NReturn : public NStatement {
public:
  NExpression &exp;
  NReturn(NExpression &exp) : exp(exp) {}
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

class NExpressionStatement : public NStatement {
public:
  NExpression &exp;
  NExpressionStatement(NExpression &exp) : exp(exp) {}
  // virtual llvm::Value *codeGen(CodeGenContext &context);
  virtual void print(std::ostream &) const;
};

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
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

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
  // virtual llvm::Value *code_generate(CodeGenContext &);
  virtual void print(std::ostream &) const;
};

#endif
