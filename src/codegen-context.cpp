#include "ast.hpp"
#include "codegen.hpp"
#include "parser.hpp"

llvm::LLVMContext LLVM_CTX;
llvm::IRBuilder<> BUILDER(LLVM_CTX);

llvm::Type *DOUBLE_TYPE = llvm::Type::getDoubleTy(LLVM_CTX);
llvm::Type *STRING_TYPE = llvm::Type::getInt8PtrTy(LLVM_CTX);
llvm::Type *INTEGER_TYPE = llvm::Type::getInt64Ty(LLVM_CTX);

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
  std::cout << std::endl;
}

void CodeGenContext::print_llvm_ir() { module->print(llvm::outs(), nullptr); }

void CodeGenContext::print_llvm_ir_to_file(std::string &filename) {
  std::error_code error_code;
  llvm::raw_fd_ostream ost(filename, error_code, llvm::sys::fs::F_None);
  module->print(ost, nullptr);
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

int CodeGenContext::write_object(std::string &filename) {
  auto target_triple = llvm::sys::getDefaultTargetTriple();
  module->setTargetTriple(target_triple);

  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(target_triple, err);
  if (!target) {
    llvm::errs() << err;
    return 1;
  }

  llvm::TargetOptions opt;
  auto reloc_model = llvm::Optional<llvm::Reloc::Model>();
  llvm::TargetMachine *target_machine = target->createTargetMachine(
      target_triple, "generic", "", opt, reloc_model);

  module->setDataLayout(target_machine->createDataLayout());

  std::error_code error_code;
  llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

  if (error_code) {
    llvm::errs() << "File open error { " << filename
                 << " }: " << error_code.message();
    return 1;
  }

  llvm::legacy::PassManager pass;
  auto FileType = llvm::CGFT_ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    llvm::errs() << "Target machine configuration unable to emit object code";
    return 1;
  }

  pass.run(*module);
  dest.flush();

  llvm::outs() << "Object code written to { " + filename + " }\n";

  return 0;
}
