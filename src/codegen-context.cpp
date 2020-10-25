#include "ast.hpp"
#include "codegen.hpp"
#include "parser.hpp"

/**
 * Name: LLVM_CTX
 * Construct: Global variable
 * Desc: The LLVM context of the program/module
 */
llvm::LLVMContext LLVM_CTX;

/**
 * Name: BUILDER
 * Construct: Global variable
 * Desc: The LLVM IR building utility constructed under the global context
 *   (see `LLVM_CTX`)
 */
llvm::IRBuilder<> BUILDER(LLVM_CTX);

/**
 * Name: DOUBLE_TYPE, STRING_TYPE, INTEGER_TYPE
 * Construct: Global variable
 * Desc: To save from frequently calling to `llvm::Type::getXXXTy(LLVM_CTX)`,
 *   these are created once at compiler startup
 */
llvm::Type *DOUBLE_TYPE = llvm::Type::getDoubleTy(LLVM_CTX);
llvm::Type *STRING_TYPE = llvm::Type::getInt8PtrTy(LLVM_CTX);
llvm::Type *INTEGER_TYPE = llvm::Type::getInt64Ty(LLVM_CTX);

/**
 * Name: get_i8_str_ptr
 * Construct: Function
 * Desc: Create a global string and return a pointer to it, used to naively
 *   implement strings in the language
 * Args:
 *   - str: The string to store as a global in the resulting program
 *   - twine: The string used to refer to the global string in the resulting
 *     program
 */
llvm::Constant *get_i8_str_ptr(char const *str, llvm::Twine const &twine) {
  return BUILDER.CreateGlobalStringPtr(str, twine);
}

CodeGenContext::CodeGenContext(std::string module_name) {
  module = new llvm::Module(module_name, LLVM_CTX);
  printf_function = create_fn_printf();
}

/**
 * Name: CodeGenContext::create_fn_printf
 * Construct: Method
 * Desc: Create the reference to the `printf` function will can be called by
 *   the Sood program and requires linking to libc to run as an executable
 */
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

/**
 * Name: CodeGenContext::code_generate
 * Construct: Method
 * Desc: Core function used to populate the module based on the AST generated
 *   earlier
 * Args:
 *   - root: The root block (see `NBlock`) of the AST
 */
void CodeGenContext::code_generate(NBlock &root) {
  std::vector<llvm::Type *> arg_types;

  llvm::FunctionType *fn_type = llvm::FunctionType::get(
      llvm::Type::getVoidTy(LLVM_CTX), makeArrayRef(arg_types), false);

  fn_main = llvm::Function::Create(fn_type, llvm::GlobalValue::ExternalLinkage,
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

/**
 * Name: CodeGenContext::verify_module
 * Construct: Method
 * Desc: Optionally verify the LLVM module
 */
void CodeGenContext::verify_module() {
  llvm::verifyModule(*module, &llvm::outs());
}

/**
 * Name: CodeGenContext::print_llvm_ir
 * Construct: Method
 * Desc: Prints the generated LLVM IR to the console
 */
void CodeGenContext::print_llvm_ir() { module->print(llvm::outs(), nullptr); }

/**
 * Name: CodeGenContext::print_llvm_ir_to_file
 * Construct: Method
 * Desc: Writes the generated LLVM IR to the specified file
 * Args:
 *   - filename: String reference to the filename
 */
void CodeGenContext::print_llvm_ir_to_file(std::string &filename) {
  std::error_code error_code;
  llvm::raw_fd_ostream ost(filename, error_code, llvm::sys::fs::F_None);
  module->print(ost, nullptr);
}

/**
 * Name: CodeGenContext::code_run
 * Construct: Method
 * Desc: Runs the main function of the module with the LLVM execution engine
 */
llvm::GenericValue CodeGenContext::code_run() {
  llvm::ExecutionEngine *engine =
      llvm::EngineBuilder(std::unique_ptr<llvm::Module>(module)).create();
  engine->finalizeObject();
  std::vector<llvm::GenericValue> no_args;
  llvm::GenericValue v = engine->runFunction(fn_main, no_args);
  return v;
}

/**
 * Name: CodeGenContext::write_object
 * Construct: Method
 * Desc: Optionally write module, as native object code, to the specified
 *   filename
 * Args:
 *   - filename: String reference to the filename
 */
int CodeGenContext::write_object(std::string &filename) {

  llvm::InitializeAllTargetInfos();
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmParsers();
  llvm::InitializeAllAsmPrinters();

  std::string target_triple = llvm::sys::getDefaultTargetTriple();
  module->setTargetTriple(target_triple);

  std::string err;
  const llvm::Target *target =
      llvm::TargetRegistry::lookupTarget(target_triple, err);
  if (!target) {
    llvm::errs() << err;
    return 1;
  }

  /**
   * Set up target machine with a generic CPU and the
   *   [PIC](https://docs.oracle.com/cd/E26505_01/html/E26506/glmqp.html)
   *   relocation model
   */
  llvm::TargetOptions opt;
  llvm::TargetMachine *target_machine = target->createTargetMachine(
      target_triple, "generic", "", opt, llvm::Reloc::PIC_);

  module->setDataLayout(target_machine->createDataLayout());

  std::error_code error_code;
  llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

  if (error_code) {
    llvm::errs() << "File open error { " << filename
                 << " }: " << error_code.message();
    return 1;
  }

  llvm::legacy::PassManager pass;
  llvm::CodeGenFileType file_type = llvm::CGFT_ObjectFile;

  if (target_machine->addPassesToEmitFile(pass, dest, nullptr, file_type)) {
    llvm::errs() << "Target machine configuration unable to emit object code";
    return 1;
  }

  pass.run(*module);
  dest.flush();

  llvm::outs() << "LLVM: Object code written to { " + filename + " }\n";

  return 0;
}
