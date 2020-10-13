#include <iostream>
#include <llvm/Support/TargetSelect.h>

#include "ast.hpp"

extern int yyparse();
extern NBlock* prg;

int main() {

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmParser();
  llvm::InitializeNativeTargetAsmPrinter();

	yyparse();

  std::cout << *prg << std::endl;

	// CodeGenContext context;
	// createCoreFunctions(context);
	// context.generateCode(*programBlock);
	// context.runCode();

	return 0;
}
