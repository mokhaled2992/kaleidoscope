#include "compiler.h"

// #include "llvm/ADT/STLExtras.h"
// #include "llvm/ADT/iterator_range.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
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
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mk
{

Compiler::Compiler()
{
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    // llvm::sys::DynamicLibrary::LoadLibraryPermanently(nullptr);
}

Compiler::~Compiler() = default;

void Compiler::operator()() {}


void Compiler::execute(llvm::Module & module)
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