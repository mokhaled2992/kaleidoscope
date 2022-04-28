#include "driver.h"

#include "compiler/codegen/codegen.h"
#include "compiler/lexer/lexer.h"
#include "compiler/parser/parser.h"

#include "util/lld.h"

#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Linker/Linker.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "lld/Common/Driver.h"


#include "fmt/core.h"


#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace mk
{

Driver::Driver() = default;

Driver::~Driver() = default;

std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
Driver::compile(const std::string_view & src) const
{
    Lexer lexer(src);
    Parser parser(lexer);
    CodeGen codegen(parser.parse());

    std::unique_ptr<llvm::Module> module(llvm::CloneModule(*codegen()));

    return std::pair{std::move(codegen).LLVMContext(), std::move(module)};
}

std::unique_ptr<llvm::TargetMachine> Driver::target(llvm::Module & ir) const
{
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
        return nullptr;
    }

    llvm::TargetOptions Options;
    // Options.NoFramePointerElim = true;
    Options.MCOptions.AsmVerbose = true;

    const auto CPU = "generic";
    const auto Features = "";
    std::unique_ptr<llvm::TargetMachine> target_machine(
        target->createTargetMachine(triple,
                                    CPU,
                                    Features,
                                    Options,
                                    llvm::Reloc::PIC_,
                                    llvm::CodeModel::Large,
                                    llvm::CodeGenOpt::Default,
                                    true));

    ir.setDataLayout(target_machine->createDataLayout());
    ir.setTargetTriple(triple);

    return target_machine;
}

std::variant<std::monostate, int64_t, double, char, void *>
Driver::execute(const llvm::Module & module) const
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

    auto main = execution_engine->getFunctionAddress("main");
    const auto main_signature = execution_engine->FindFunctionNamed("main");
    if (!main || !main_signature)
    {
        return std::monostate{};
    }

    const auto return_type = main_signature->getReturnType();

    if (return_type->isIntegerTy(64))
    {
        return reinterpret_cast<int64_t (*)()>(main)();
    }
    else if (return_type->isDoubleTy())
    {
        return reinterpret_cast<double (*)()>(main)();
    }
    else if (return_type->isPointerTy())
    {
        return reinterpret_cast<void * (*)()>(main)();
    }
    else if (return_type->isVoidTy())
    {
        reinterpret_cast<void (*)()>(main)();
    }
    return std::monostate{};
}

std::variant<std::monostate, int64_t, double, char, void *>
Driver::operator()(const std::string_view & src, Execute)
{
    const auto [context, ir] = compile(src);

    const auto t = target(*ir);

    return execute(*ir);
}

std::pair<std::unique_ptr<llvm::LLVMContext>, std::unique_ptr<llvm::Module>>
Driver::operator()(const std::vector<std::string_view> & srcs, Link)
{
    // This workaround writes the module + context into a bitcode stream then
    // re-loads the module with a given context
    const auto merge =
        [](const llvm::Module & ir, llvm::LLVMContext & context)  //
    {
        std::string s;
        llvm::raw_string_ostream stream(s);
        llvm::WriteBitcodeToFile(ir, stream);
        return std::move(
            *llvm::parseBitcodeFile(*llvm::MemoryBuffer::getMemBuffer(s),
                                    context));
    };

    auto context = std::make_unique<llvm::LLVMContext>();
    auto module = std::make_unique<llvm::Module>("test", *context);
    llvm::Linker linker(*module);

    for (const auto & src : srcs)
    {
        auto [_, ir] = compile(src);

        const auto t = target(*ir);

        if (linker.linkInModule(merge(*ir, *context)))
            return std::pair{nullptr, nullptr};
    }

    return std::pair{std::move(context), std::move(module)};
}

void Driver::operator()(const std::string_view & src,
                        const Object::Args & args) const
{
    auto [_, ir] = compile(src);

    const auto t = target(*ir);

    std::error_code EC;
    llvm::raw_fd_ostream dest(fmt::format("{}.o", args.outfile),
                              EC,
                              llvm::sys::fs::OpenFlags::OF_None);

    if (EC)
        return;

    llvm::legacy::PassManager pass;

    llvm::TargetLibraryInfoImpl TLII(llvm::Triple(t->getTargetTriple()));
    pass.add(new llvm::TargetLibraryInfoWrapperPass(TLII));

    if (t->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile))
        return;

    pass.run(*ir);
    dest.flush();
}

void Driver::operator()(const std::string_view & src, Bitcode) const
{
    auto [_, ir] = compile(src);

    const auto t = target(*ir);

    std::error_code EC;
    llvm::raw_fd_ostream stream("output.bc",
                                EC,
                                llvm::sys::fs::OpenFlags::OF_None);
    llvm::WriteBitcodeToFile(*ir, stream);
}

bool Driver::operator()(const std::string_view & src,
                        const Library::Shared::Args & args) const
{

    (*this)(src, Object::Args{.outfile = args.outfile});

    std::string s;
    llvm::raw_string_ostream stream(s);
    std::vector<std::string> raw_args = {"ld",
                                         "-shared",
                                         fmt::format("-dynamic-linker={}",
                                                     args.dynamic_linker),
                                         fmt::format("-o{}.so", args.outfile)};

    for (const auto & l : args.link_paths)
        raw_args.emplace_back(fmt::format("-L{}", l));

    raw_args.emplace_back(fmt::format("{}.o", args.outfile));
    std::copy(args.link_objects.cbegin(),
              args.link_objects.cend(),
              std::back_inserter(raw_args));

    std::vector<const char *> lld_args;
    lld_args.reserve(raw_args.size());
    for (const auto & arg : raw_args)
        lld_args.emplace_back(arg.data());
    return lld::elf::ScopedLink{}(lld_args, stream, stream, false, false);
}

}  // namespace mk