#ifndef __COMPILER_H__
#define __COMPILER_H__

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"

#include <memory>
#include <vector>

namespace llvm
{
class TargetMachine;
class SymbolResolver;
};  // namespace llvm

namespace mk
{
class Compiler
{

public:
    Compiler();
    ~Compiler();

    void operator()();
    void execute(llvm::Module & module);

    // ModuleHandle addModule(std::unique_ptr<Module> M);
    // void removeModule(ModuleHandle K);
    // llvm::JITSymbol findSymbol(const std::string Name);

private:
    // std::string mangle(const std::string & Name);
    // llvm::JITSymbol findMangledSymbol(const std::string & Name);

    // std::unique_ptr<ExecutionEngine> execution_engine;

    // std::shared_ptr<llvm::SymbolResolver> resolver;
    // std::unique_ptr<llvm::TargetMachine> target_machine;
    // const llvm::DataLayout data_layout;
    // llvm::orc::ObjectLinkingLayer object_layer;
    // llvm::orc::IRCompileLayer compiler_layer;
    // std::vector<ModuleHandle> module_keys;
};
}  // namespace mk

#endif