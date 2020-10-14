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
  out << indent() << "assignment { lhs: " << lhs << ", rhs: ";
  inc_ilvl();
  out << rhs;
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NBinaryExpression::print(std::ostream &out) const {
  out << '\n'
      << indent() << "binary_expression { lhs: " << lhs << ", op: " << op
      << ", rhs: ";
  inc_ilvl(2);
  out << rhs << " }";
  dec_ilvl(2);
}

void NBlock::print(std::ostream &out) const {
  out << indent() << "block {" << '\n';
  inc_ilvl();
  for (auto &stmt : stmts) {
    out << *stmt;
    if (&stmt != &stmts.back())
      out << '\n';
  }
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NExpressionStatement::print(std::ostream &out) const {
  out << exp << '\n';
}

void NFloat::print(std::ostream &out) const {
  // No `indent()`
  out << "float(" << val << ")";
}

void NFunctionCall::print(std::ostream &out) const {
  out << '\n' << indent() << "func_call { id: " << func;
  if (args.size()) {
    out << ", args: { " << '\n';
    inc_ilvl(2);
    out << indent();
    for (auto &arg : args)
      out << *arg << ", ";
    dec_ilvl(2);
    out << "}";
  }
  out << " }" << '\n';
}

void NElseStatement::print(std::ostream &out) const {
  out << indent() << "{" << '\n';
  inc_ilvl();
  out << block;
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NIfStatement::print(std::ostream &out) const {
  out << indent() << "if { cond: ";
  inc_ilvl(2);
  out << cond;
  dec_ilvl();
  out << "," << '\n' << block;
  dec_ilvl();
  out << indent() << "}";
  if (els)
    out << '\n' << indent() << "else \\" << '\n' << *els;
}

void NUntilStatement::print(std::ostream &out) const {
  out << indent() << "until { cond: ";
  inc_ilvl(2);
  out << cond;
  dec_ilvl();
  out << "," << '\n' << block;
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NWhileStatement::print(std::ostream &out) const {
  out << indent() << "while { cond: ";
  inc_ilvl(2);
  out << cond;
  dec_ilvl();
  out << "," << '\n' << block;
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NFunctionDeclaration::print(std::ostream &out) const {
  out << indent() << "func_decl { type: " << type << ", name: " << id << ", ";
  inc_ilvl();
  if (args.size()) {
    out << "args: { " << '\n';
    inc_ilvl();
    for (auto &arg : args)
      out << *arg;
    dec_ilvl();
    out << indent() << "}, ";
  }
  out << '\n' << block;
  dec_ilvl();
  out << indent() << "}" << '\n';
}

void NIdentifier::print(std::ostream &out) const {
  out << "ident(" << val << ")";
}

void NInteger::print(std::ostream &out) const { out << "int(" << val << ")"; }

void NRead::print(std::ostream &out) const {
  out << indent() << "read { from: " << from << ", to: " << to << " }" << '\n';
}

void NWrite::print(std::ostream &out) const {
  out << indent() << "write { exp: " << exp << ", to: " << to << " }" << '\n';
}

void NReturn::print(std::ostream &out) const {
  out << indent() << "return { exp: " << exp << " }" << '\n';
}

void NString::print(std::ostream &out) const {
  out << "str(" << val.c_str() << ")";
}

void NUnaryExpression::print(std::ostream &out) const {
  out << "unary_expression { op: " << op << ", exp: " << rhs << " }" << '\n';
}

void NVariableDeclaration::print(std::ostream &out) const {
  out << indent() << "var_decl { type: " << type << ", lhs: " << lhs;
  if (rhs) {
    out << ", rhs: ";
    inc_ilvl(2);
    out << *rhs;
    dec_ilvl(2);
  }
  out << " }" << '\n';
}
