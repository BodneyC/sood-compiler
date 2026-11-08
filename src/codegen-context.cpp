#include "ast.hpp"
#include "codegen.hpp"

#include <llvm/Bitcode/BitcodeReader.h>
#include <llvm/Bitcode/BitcodeWriter.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>

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
 * Name: BOOLEAN_TYPE, DOUBLE_TYPE, STRING_TYPE, INTEGER_TYPE, VOID_TYPE
 * Construct: Global variable
 * Desc: To save from frequently calling to `llvm::Type::getXXXTy(LLVM_CTX)`,
 *   these are created once at compiler startup
 * Notes:
 *   - Booleans are not a declarable type, they are the result of comparisons
 *     and the `true`/`false` literals
 */
llvm::Type *BOOLEAN_TYPE = llvm::Type::getInt1Ty(LLVM_CTX);
llvm::Type *DOUBLE_TYPE = llvm::Type::getDoubleTy(LLVM_CTX);
llvm::Type *STRING_TYPE = llvm::PointerType::getUnqual(LLVM_CTX);
llvm::Type *INTEGER_TYPE = llvm::Type::getInt64Ty(LLVM_CTX);
llvm::Type *VOID_TYPE = llvm::Type::getVoidTy(LLVM_CTX);

/**
 * Name: CodeGenContext::CodeGenContext
 * Construct: Constructor
 * Desc: Creates the module for the native target and declares the libc
 *   functions used by the generated code
 * Args:
 *   - module_name: The name of the LLVM module
 */
CodeGenContext::CodeGenContext(std::string module_name) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();

  module = std::make_unique<llvm::Module>(module_name, LLVM_CTX);

  /**
   * Set up target machine with a generic CPU and the
   *   [PIC](https://docs.oracle.com/cd/E26505_01/html/E26506/glmqp.html)
   *   relocation model
   */
  llvm::Triple triple(llvm::sys::getDefaultTargetTriple());
  std::string err;
  const llvm::Target *target = llvm::TargetRegistry::lookupTarget(triple, err);
  if (!target)
    throw CodeGenException("Could not find native target: " + err);
  target_machine.reset(target->createTargetMachine(
      triple, "generic", "", llvm::TargetOptions(), llvm::Reloc::PIC_));
  module->setTargetTriple(triple);
  module->setDataLayout(target_machine->createDataLayout());
}

CodeGenContext::~CodeGenContext() {}

/**
 * Name: LIBC_FUNCTIONS
 * Construct: Global variable
 * Desc: The libc functions which may be called by the generated code, these
 *   require linking to libc to run as an executable
 */
static const std::map<std::string, llvm::FunctionType *> LIBC_FUNCTIONS = {
    {"dprintf", llvm::FunctionType::get(BUILDER.getInt32Ty(),
                                        {BUILDER.getInt32Ty(), STRING_TYPE},
                                        true)},
    {"scanf",
     llvm::FunctionType::get(BUILDER.getInt32Ty(), {STRING_TYPE}, true)},
    {"strcmp", llvm::FunctionType::get(BUILDER.getInt32Ty(),
                                       {STRING_TYPE, STRING_TYPE}, false)},
    {"malloc", llvm::FunctionType::get(STRING_TYPE, {INTEGER_TYPE}, false)},
};

/**
 * Name: CodeGenContext::get_libc
 * Construct: Method
 * Desc: Returns the declaration of a libc function (see `LIBC_FUNCTIONS`),
 *   declaring it on first use so that unused functions are left out
 * Args:
 *   - name: The name of the libc function
 */
llvm::Function *CodeGenContext::get_libc(const std::string &name) {
  if (llvm::Function *func = module->getFunction(name))
    return func;
  llvm::Function *func =
      llvm::Function::Create(LIBC_FUNCTIONS.at(name),
                             llvm::Function::ExternalLinkage, name, module.get());
  func->setCallingConv(llvm::CallingConv::C);
  return func;
}

/**
 * Name: CodeGenContext::is_reserved
 * Construct: Method
 * Desc: Whether a function name is unavailable to Sood functions as it is
 *   used by the generated code
 * Args:
 *   - name: The function name
 */
bool CodeGenContext::is_reserved(const std::string &name) {
  return name == "main" || LIBC_FUNCTIONS.count(name);
}

/**
 * Name: CodeGenContext::find_local
 * Construct: Method
 * Desc: Looks up a variable from the innermost scope outwards, stopping at
 *   the enclosing function's scope, then falls back to the global scope
 * Args:
 *   - name: The name of the variable
 *   - out: Set to the variable's pointer and type if found
 */
bool CodeGenContext::find_local(const std::string &name, ValTypeTuple &out) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    auto found = it->locals.find(name);
    if (found != it->locals.end()) {
      out = found->second;
      return true;
    }
    if (it->fn_boundary)
      break;
  }
  auto found = scopes.front().locals.find(name);
  if (found == scopes.front().locals.end())
    return false;
  out = found->second;
  return true;
}

/**
 * Name: CodeGenContext::string_constant
 * Construct: Method
 * Desc: Returns a pointer to a global, constant, null-terminated string;
 *   identical strings share one global
 * Args:
 *   - str: The string to store as a global in the resulting program
 */
llvm::Constant *CodeGenContext::string_constant(const std::string &str) {
  auto found = string_constants.find(str);
  if (found != string_constants.end())
    return found->second;
  llvm::Constant *global = BUILDER.CreateGlobalString(str, "str", 0, module.get());
  string_constants[str] = global;
  return global;
}

/**
 * Name: CodeGenContext::create_variable
 * Construct: Method
 * Desc: Creates storage for a variable in the current scope. Variables in the
 *   global scope are LLVM globals (so functions can use them), others are
 *   stack allocations in the entry block of the current function, which
 *   avoids growing the stack when declared in a loop and lets LLVM promote
 *   them to registers
 * Args:
 *   - name: The name of the variable
 *   - type: The type of the variable
 */
llvm::Value *CodeGenContext::create_variable(const std::string &name,
                                             llvm::Type *type) {
  llvm::Value *var;
  if (in_global_scope()) {
    var = new llvm::GlobalVariable(*module, type, false,
                                   llvm::GlobalValue::InternalLinkage,
                                   zero_value_for(*this, type), name);
  } else {
    llvm::BasicBlock &entry = current_function()->getEntryBlock();
    llvm::IRBuilder<> entry_builder(&entry, entry.begin());
    var = entry_builder.CreateAlloca(type, nullptr, name);
  }
  set_local(name, var, type);
  return var;
}

/**
 * Name: CodeGenContext::branch_to
 * Construct: Method
 * Desc: Branches to the given block, unless the current block has already
 *   been terminated (e.g. by a `return`)
 * Args:
 *   - block: The block to which to branch
 */
void CodeGenContext::branch_to(llvm::BasicBlock *block) {
  if (!BUILDER.GetInsertBlock()->hasTerminator())
    BUILDER.CreateBr(block);
}

/**
 * Name: CodeGenContext::code_generate
 * Construct: Method
 * Desc: Core function used to populate the module based on the AST generated
 *   earlier, the top-level statements form the body of `main`
 * Args:
 *   - root: The root block (see `NBlock`) of the AST
 */
void CodeGenContext::code_generate(NBlock &root) {
  llvm::FunctionType *fn_type =
      llvm::FunctionType::get(BUILDER.getInt32Ty(), false);
  fn_main = llvm::Function::Create(fn_type, llvm::GlobalValue::ExternalLinkage,
                                   "main", module.get());
  BUILDER.SetInsertPoint(llvm::BasicBlock::Create(LLVM_CTX, "entry", fn_main));

  push_scope(true);

  /** Declare all top-level functions so they can be called before their
   * declaration */
  for (NStatement *stmt : root.stmts) {
    if (auto *fn = dynamic_cast<NFunctionDeclaration *>(stmt)) {
      try {
        fn->declare(*this);
      } catch (CodeGenException &e) {
        e.line = fn->line;
        throw;
      }
    }
  }

  root.code_generate(*this);
  if (!BUILDER.GetInsertBlock()->hasTerminator())
    BUILDER.CreateRet(BUILDER.getInt32(0));

  pop_scope();
}

/**
 * Name: CodeGenContext::verify_module
 * Construct: Method
 * Desc: Verify the LLVM module, printing any errors, returns true if valid
 */
bool CodeGenContext::verify_module() {
  return !llvm::verifyModule(*module, &llvm::errs());
}

/**
 * Name: CodeGenContext::optimize
 * Construct: Method
 * Desc: Runs LLVM's default optimization pipeline over the module
 * Args:
 *   - level: The optimization level, 0 to 3 (0 does nothing)
 */
void CodeGenContext::optimize(unsigned level) {
  if (level == 0)
    return;

  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  llvm::PassBuilder pb(target_machine.get());
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  llvm::OptimizationLevel opt_level = level == 1   ? llvm::OptimizationLevel::O1
                                      : level == 2 ? llvm::OptimizationLevel::O2
                                                   : llvm::OptimizationLevel::O3;
  pb.buildPerModuleDefaultPipeline(opt_level).run(*module, mam);
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
  llvm::raw_fd_ostream ost(filename, error_code, llvm::sys::fs::OF_None);
  if (error_code)
    throw CodeGenException("Could not open " + filename + ": " +
                           error_code.message());
  module->print(ost, nullptr);
}

/**
 * Name: CodeGenContext::code_run
 * Construct: Method
 * Desc: Runs the main function of the module with LLVM's JIT, returning its
 *   exit status
 * Notes:
 *   - The JIT takes ownership of the module and its context, so it is given
 *     a copy (via bitcode) leaving this module usable afterwards
 */
int CodeGenContext::code_run() {
  llvm::SmallVector<char, 0> bitcode;
  llvm::raw_svector_ostream bitcode_ost(bitcode);
  llvm::WriteBitcodeToFile(*module, bitcode_ost);

  auto jit_ctx = std::make_unique<llvm::LLVMContext>();
  auto jit_module = llvm::parseBitcodeFile(
      llvm::MemoryBufferRef(llvm::StringRef(bitcode.data(), bitcode.size()),
                            module->getName()),
      *jit_ctx);
  if (!jit_module)
    throw CodeGenException("Could not copy module for JIT: " +
                           llvm::toString(jit_module.takeError()));

  auto jit = llvm::orc::LLJITBuilder().create();
  if (!jit)
    throw CodeGenException("Could not create JIT: " +
                           llvm::toString(jit.takeError()));

  if (auto err = (*jit)->addIRModule(llvm::orc::ThreadSafeModule(
          std::move(*jit_module), std::move(jit_ctx))))
    throw CodeGenException("Could not add module to JIT: " +
                           llvm::toString(std::move(err)));

  auto main_sym = (*jit)->lookup("main");
  if (!main_sym)
    throw CodeGenException("Could not find `main` in JIT: " +
                           llvm::toString(main_sym.takeError()));

  return main_sym->toPtr<int (*)()>()();
}

/**
 * Name: CodeGenContext::write_object
 * Construct: Method
 * Desc: Write module, as native object code, to the specified filename
 * Args:
 *   - filename: String reference to the filename
 */
int CodeGenContext::write_object(std::string &filename) {
  std::error_code error_code;
  llvm::raw_fd_ostream dest(filename, error_code, llvm::sys::fs::OF_None);

  if (error_code) {
    llvm::errs() << "File open error { " << filename
                 << " }: " << error_code.message() << "\n";
    return 1;
  }

  llvm::legacy::PassManager pass;
  if (target_machine->addPassesToEmitFile(pass, dest, nullptr,
                                          llvm::CodeGenFileType::ObjectFile)) {
    llvm::errs() << "Target machine configuration unable to emit object code\n";
    return 1;
  }

  pass.run(*module);
  dest.flush();

  return 0;
}
