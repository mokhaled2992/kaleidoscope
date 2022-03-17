#include "driver.h"

#include "compiler/codegen/codegen.h"
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"

// #include "llvm/ADT/STLExtras.h"
// #include "llvm/ADT/iterator_range.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Utils/Cloning.h"

// #include "llvm/ExecutionEngine/JITSymbol.h"
// #include "llvm/ExecutionEngine/Orc/CompileUtils.h"
// #include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
// #include "llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h"
// #include "llvm/ExecutionEngine/RTDyldMemoryManager.h"
// #include "llvm/IR/DataLayout.h"
// #include "llvm/IR/Mangler.h"
// #include "llvm/Support/DynamicLibrary.h"
// #include "llvm/Support/raw_ostream.h"
// #include "llvm/Target/TargetMachine.h"


#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mk
{

Driver::Driver()
{
    // llvm::InitializeNativeTarget();
    // llvm::InitializeNativeTargetAsmPrinter();
    // llvm::InitializeNativeTargetAsmParser();
    // llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

Driver::~Driver() = default;

void Driver::operator()(const std::string_view & src)
{

    Lexer lexer(src);
    Parser parser(lexer);
    CodeGen codegen(parser.parse());

    std::unique_ptr<llvm::Module> module(llvm::CloneModule(*codegen()));

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();

    std::string Error;
    const auto triple = llvm::sys::getDefaultTargetTriple();

    // Get the target specific parser.
    const llvm::Target * target =
        llvm::TargetRegistry::lookupTarget(triple, Error);
    if (!target)
    {
        std::cerr << Error << std::endl;
        return;
    }

    llvm::TargetOptions Options;
    // Options.NoFramePointerElim = true;
    Options.MCOptions.AsmVerbose = true;

    std::string MCPU = "";  // don't target specific cpu
    std::string FeaturesStr = "";
    std::unique_ptr<llvm::TargetMachine> target_machine(
        target->createTargetMachine(triple,
                                    MCPU,
                                    FeaturesStr,
                                    Options,
                                    llvm::Reloc::PIC_,
                                    llvm::CodeModel::Large,
                                    llvm::CodeGenOpt::Default,
                                    true));

    module->setDataLayout(target_machine->createDataLayout());

    execute(*module);
}


void Driver::execute(const llvm::Module & module)
{
    std::string llvm_errors;
    std::unique_ptr<llvm::ExecutionEngine> execution_engine(
        llvm::EngineBuilder(
            std::unique_ptr<llvm::Module>(llvm::CloneModule(module)))
            .setErrorStr(&llvm_errors)
            .setEngineKind(llvm::EngineKind::JIT)
            .setMCJITMemoryManager(
                std::make_unique<llvm::SectionMemoryManager>())
            .setVerifyModules(true)
            .setOptLevel(llvm::CodeGenOpt::Default)
            .create());
}
// ModuleHandle Compiler::addModule(std::unique_ptr<Module> M)
// {
//     auto K = execution_session.allocateVModule();
//     cantFail(compiler_layer.addModule(K, std::move(M)));
//     module_keys.push_back(K);
//     return K;
// }

// void Compiler::removeModule(ModuleHandle K)
// {
//     module_keys.erase(find(module_keys, K));
//     compiler_layer.removeModule(K);
// }

// llvm::JITSymbol Compiler::findSymbol(const std::string Name)
// {
//     return findMangledSymbol(mangle(Name));
// }

// std::string Compiler::mangle(const std::string & Name)
// {
//     std::string MangledName;
//     {
//         llvm::raw_string_ostream MangledNameStream(MangledName);
//         llvm::Mangler::getNameWithPrefix(MangledNameStream, Name, DL);
//     }
//     return MangledName;
// }

// llvm::JITSymbol Compiler::findMangledSymbol(const std::string & Name)
// {
//     const bool ExportedSymbolsOnly = true;
//     // Search modules in reverse order: from last added to first added.
//     // This is the opposite of the usual search order for dlsym, but makes
//     more
//     // sense in a REPL where we want to bind to the newest available
//     definition. for (auto H : llvm::make_range(ModuleKeys.rbegin(),
//     ModuleKeys.rend()))
//         if (auto Sym =
//                 compiler_layer.findSymbolIn(H, Name, ExportedSymbolsOnly))
//             return Sym;

//     // If we can't find the symbol in the JIT, try looking in the host
//     process. if (auto SymAddr =
//             llvm::RTDyldMemoryManager::getSymbolAddressInProcess(Name))
//         return llvm::JITSymbol(SymAddr, llvm::JITSymbolFlags::Exported);

//     return nullptr;
// }
}  // namespace mk