#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JITSymbol.h"
#include "llvm/ExecutionEngine/Orc/Core.h"
#include "llvm/ExecutionEngine/Orc/IRCompileLayer.h"
#include "llvm/ExecutionEngine/Orc/ObjectLinkingLayer.h"
#include "llvm/IR/DataLayout.h"

#include "fmt/format.h"

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
    };
    struct Elf
    {
        struct Args
        {
            std::string dynamic_linker;
            std::string outfile;
            std::vector<std::string> paths;
            std::vector<std::string> objects;
            std::vector<std::string> libs;

            virtual std::string extension() const = 0;
            virtual std::vector<std::string> additional_flags() const
            {
                return {};
            }
            virtual ~Args() = 0;
        };
    };
    struct Library
    {
        struct Static
        {
        };
        struct Shared
        {
            struct Args final : Elf::Args
            {
                std::string extension() const override { return "so"; }
                std::vector<std::string> additional_flags() const override
                {
                    return {"-shared"};
                }
                ~Args() override = default;
            };
        };
    };
    struct Executable
    {
        struct Args final : Elf::Args
        {
            std::string rpath;

            std::string extension() const override { return "out"; }
            std::vector<std::string> additional_flags() const override
            {
                return {fmt::format("-rpath={}", rpath)};
            }
            ~Args() override = default;
        };
    };

    struct Compile
    {
    };
    struct IR
    {
        struct Args
        {
            std::string outfile;
        };
    };
    struct Bitcode
    {
        struct Args
        {
            std::string outfile;
        };
    };
    struct Object
    {
        struct Args
        {
            std::string outfile;
        };
    };

    Driver();

    ~Driver();

    std::variant<std::monostate, int64_t, int32_t, double, char, void *>
    operator()(const std::string_view & src, Execute);

    std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
    operator()(const std::vector<std::string_view> & srcs, Link);

    void operator()(const std::string_view & src, const Object::Args &) const;
    void operator()(const std::string_view & src, const Bitcode::Args &) const;

    bool operator()(const std::string_view & src,
                    const Library::Shared::Args &) const;

    bool operator()(const std::string_view & src,
                    const Executable::Args &) const;

    void operator()(const std::string_view & src, const IR::Args &) const;

private:
    bool operator()(const std::string_view & src, const Elf::Args &) const;

    std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
    compile(const std::string_view & src) const;

    std::unique_ptr<llvm::TargetMachine> target(llvm::Module & ir) const;

    std::variant<std::monostate, int64_t, int32_t, double, char, void *>
    execute(const llvm::Module & module) const;
};
}  // namespace mk

#endif