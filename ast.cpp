#include "ast.hpp"

// Yes, this is all bad practice...

const int INDENT_WIDTH = 2;
int ilvl = 0;

std::string indent() { return std::string(ilvl, ' '); }
void inc_ilvl(int num = 1) { ilvl += (num * INDENT_WIDTH); }
void dec_ilvl(int num = 1) {
  int width = num * INDENT_WIDTH;
  if (ilvl >= width)
    ilvl -= width;
}

void NAssignment::print(std::ostream &out) const {
  std::cout << indent() << "assignment { lhs: " << lhs << ", rhs: ";
  inc_ilvl(2);
  std::cout << rhs << " }";
  dec_ilvl(2);
}

void NBinaryExpression::print(std::ostream &out) const {
  std::cout << '\n'
            << indent() << "binary_expression { lhs: " << lhs << ", op: " << op
            << ", rhs: ";
  inc_ilvl(2);
  std::cout << rhs << " }";
  dec_ilvl(2);
}

void NBlock::print(std::ostream &out) const {
  std::cout << indent() << "block {" << '\n';
  inc_ilvl();
  for (auto &stmt : stmts) {
    std::cout << *stmt;
    if (&stmt != &stmts.back())
      std::cout << '\n';
  }
  dec_ilvl();
  std::cout << indent() << "}" << '\n';
}

void NExpressionStatement::print(std::ostream &out) const {
  std::cout << exp << '\n';
}

void NFloat::print(std::ostream &out) const {
  // No `indent()`
  std::cout << "float(" << val << ")";
}

void NFunctionCall::print(std::ostream &out) const {
  std::cout << indent() << "func_call { func: " << func;
  if (args.size()) {
    std::cout << ", args: { " << '\n';
    inc_ilvl(2);
    for (auto &arg : args)
      std::cout << *arg << ", ";
    dec_ilvl(2);
    std::cout << "}";
  }
  std::cout << " }" << '\n';
}

void NFunctionDeclaration::print(std::ostream &out) const {
  std::cout << indent() << "func_decl { type: " << type << ", name: " << id
            << ", ";
  inc_ilvl();
  if (args.size()) {
    std::cout << "args: { " << '\n';
    inc_ilvl();
    for (auto &arg : args)
      std::cout << *arg;
    dec_ilvl();
    std::cout << indent() << "}, ";
  }
  std::cout << '\n' << block;
  dec_ilvl();
  std::cout << indent() << "}" << '\n';
}

void NIdentifier::print(std::ostream &out) const {
  std::cout << "ident(" << val << ")";
}

void NInteger::print(std::ostream &out) const {
  std::cout << "int(" << val << ")";
}

void NReturn::print(std::ostream &out) const {
  std::cout << "ret(" << exp << ")" << '\n';
}

void NString::print(std::ostream &out) const {
  std::cout << "str(" << val.c_str() << ")";
}

void NUnaryExpression::print(std::ostream &out) const {
  std::cout << "unary_expression { op: " << op << ", exp: " << rhs << " }"
            << '\n';
}

void NVariableDeclaration::print(std::ostream &out) const {
  std::cout << indent() << "var_decl { type: " << type << ", lhs: " << lhs;
  if (rhs) {
    std::cout << ", rhs: ";
    inc_ilvl(2);
    std::cout << *rhs;
    dec_ilvl(2);
  }
  std::cout << " }" << '\n';
}
