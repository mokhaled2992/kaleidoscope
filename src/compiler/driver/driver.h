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
    };
    struct Link
    {

        bool error;
    };
    struct Library
    {
        enum class Type
        {
            Shared,
            Static
        };
    };
    struct Compile
    {
    };
    struct IR
    {
    };

    Driver();

    ~Driver();

    std::variant<std::monostate, int64_t, double, char, void *>
    operator()(const std::string_view & src, Execute);

    std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
    operator()(const std::vector<std::string_view> & srcs, Link);

private:
    std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
    compile(const std::string_view & src) const;

    std::unique_ptr<llvm::TargetMachine> target(llvm::Module & ir) const;

    std::variant<std::monostate, int64_t, double, char, void *>
    execute(const llvm::Module & module) const;
};
}  // namespace mk

#endif