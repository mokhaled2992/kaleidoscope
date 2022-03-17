#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"

#include <memory>
#include <variant>
#include <vector>

namespace llvm
{
class TargetMachine;
class SymbolResolver;
};  // namespace llvm

namespace mk
{
class Driver
{

public:
    struct Execute
    {
        double result;
    };
    struct Link
    {
    };
    struct Library
    {
        enum class Type
        {
            shared,
            object
        };
    };
    struct Compile
    {
    };
    struct IR
    {
    };

    using Action =
        std::variant<std::monostate, Execute, Link, Library, Compile, IR>;

    Driver();
    ~Driver();

    void operator()(const std::string_view & src, Action & action);

    // ModuleHandle addModule(std::unique_ptr<Module> M);
    // void removeModule(ModuleHandle K);
    // llvm::JITSymbol findSymbol(const std::string Name);

private:
    void execute(const llvm::Module & module, Execute & execute);
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