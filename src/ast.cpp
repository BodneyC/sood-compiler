#include "ast.hpp"

// Yes, this is all bad practice...

const int INDENT_WIDTH = 2;

struct Indent {
  int ilvl = 0;
  std::string indent() { return std::string(ilvl, ' '); }
  void inc(int num = 1) { ilvl += (num * INDENT_WIDTH); }
  void dec(int num = 1) {
    int width = num * INDENT_WIDTH;
    if (ilvl >= width)
      ilvl -= width;
  }
};
Indent indt;

void NAssignment::print(std::ostream &out) const {
  out << indt.indent() << "assignment {" << '\n';
  indt.inc();
  out << indt.indent() << "lhs: " << lhs << "," << '\n';
  out << indt.indent() << "rhs: " << rhs;
  indt.dec();
  out << '\n' << indt.indent() << "}" << '\n';
}

void NBinaryExpression::print(std::ostream &out) const {
  out << "binary_expression {" << '\n';
  indt.inc();
  out << indt.indent() << "lhs: " << lhs << "," << '\n';
  out << indt.indent() << "op: " << op << "," << '\n';
  out << indt.indent() << "rhs: " << rhs << '\n';
  indt.dec();
  out << indt.indent() << "}";
}

void NBlock::print(std::ostream &out) const {
  out << indt.indent() << "block: {" << '\n';
  indt.inc();
  for (auto &stmt : stmts) {
    out << *stmt;
    if (&stmt != &stmts.back())
      out << '\n';
  }
  indt.dec();
  out << indt.indent() << "}" << '\n';
}

void NExpressionStatement::print(std::ostream &out) const {
  out << exp << '\n';
}

void NFloat::print(std::ostream &out) const {
  // No `indt.indent()`
  out << "float(" << val << ")";
}

void NFunctionCall::print(std::ostream &out) const {
  out << "func_call {" << '\n';
  indt.inc();
  out << indt.indent() << "id: " << id;
  if (args.size()) {
    out << ", args: {" << '\n';
    indt.inc();
    out << indt.indent();
    for (auto &arg : args)
      out << *arg << ", ";
    indt.dec();
    out << '\n' << indt.indent() << "}";
  }
  indt.dec();
  out << '\n' << indt.indent() << "}";
}

void NElseStatement::print(std::ostream &out) const {
  out << indt.indent() << "{" << '\n';
  indt.inc();
  out << block;
  indt.dec();
  out << indt.indent() << "}" << '\n';
}

void NIfStatement::print(std::ostream &out) const {
  out << indt.indent() << "if_stmt {" << '\n';
  indt.inc();
  out << indt.indent() << "cond: " << cond << "," << '\n';
  out << block;
  indt.dec();
  out << indt.indent() << "}";
  if (els)
    out << '\n' << indt.indent() << "else \\" << '\n' << *els;
}

void NUntilStatement::print(std::ostream &out) const {
  out << indt.indent() << "until_stmt {" << '\n';
  indt.inc();
  out << indt.indent() << "cond: " << cond << "," << '\n';
  out << block;
  indt.dec();
  out << indt.indent() << "}" << '\n';
}

void NWhileStatement::print(std::ostream &out) const {
  out << indt.indent() << "while_stmt {" << '\n';
  indt.inc();
  out << indt.indent() << "cond: " << cond << "," << '\n';
  out << block;
  indt.dec();
  out << indt.indent() << "}" << '\n';
}

void NFunctionDeclaration::print(std::ostream &out) const {
  out << indt.indent() << "func_decl {" << '\n';
  indt.inc();
  out << indt.indent() << "type: " << type << "," << '\n';
  out << indt.indent() << "name: " << id << "," << '\n';
  if (args.size()) {
    out << indt.indent() << "args: {" << '\n';
    indt.inc();
    for (auto &arg : args)
      out << *arg;
    indt.dec();
    out << indt.indent() << "}, ";
  }
  out << '\n' << block;
  indt.dec();
  out << indt.indent() << "}" << '\n';
}

void NIdentifier::print(std::ostream &out) const {
  out << "ident(" << val << ")";
}

void NInteger::print(std::ostream &out) const { out << "int(" << val << ")"; }

void NRead::print(std::ostream &out) const {
  out << indt.indent() << "read { from: " << from << ", to: " << to << " }"
      << '\n';
}

void NWrite::print(std::ostream &out) const {
  out << indt.indent() << "write { exp: " << exp << ", to: " << to << " }"
      << '\n';
}

void NReturnStatement::print(std::ostream &out) const {
  out << indt.indent() << "return { exp: " << exp << " }" << '\n';
}

void NString::print(std::ostream &out) const {
  out << "str(" << val.c_str() << ")";
}

void NUnaryExpression::print(std::ostream &out) const {
  out << "unary_expression { op: " << op << ", exp: " << rhs << " }" << '\n';
}

void NVariableDeclaration::print(std::ostream &out) const {
  out << indt.indent() << "var_decl { type: " << type << ", lhs: " << lhs;
  if (rhs) {
    out << ", rhs: ";
    indt.inc(2);
    out << *rhs;
    indt.dec(2);
  }
  out << " }" << '\n';
}
