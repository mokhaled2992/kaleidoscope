#ifndef __CODE_GEN_H__
#define __CODE_GEN_H__

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

#include "compiler/parser/visitor.h"

#include <map>
#include <memory>
#include <string_view>
#include <variant>
#include <vector>

namespace llvm
{
class Module;
class Value;
class AllocaInst;
namespace legacy
{
class FunctionPassManager;
}
}  // namespace llvm

namespace mk
{
namespace ast
{
class Node;
}
class CodeGen : private ast::Visitor
{
public:
    CodeGen(const std::vector<std::unique_ptr<ast::Node>> & root);
    ~CodeGen();
    const llvm::Module * operator()();

    std::unique_ptr<llvm::LLVMContext> LLVMContext() &&
    {
        return std::move(context);
    }

private:
    void visit(ast::Variable &) override;
    void visit(ast::Literal &) override;
    void visit(ast::UnaryExpr &) override;
    void visit(ast::BinExpr &) override;
    void visit(ast::CallExpr &) override;
    void visit(ast::ConditionalExpr &) override;
    void visit(ast::ForExpr &) override;
    void visit(ast::ProtoType &) override;
    void visit(ast::Function &) override;
    void visit(ast::LetExpr &) override;
    void visit(ast::Extern &) override;
    void visit(ast::Error &) override;

    llvm::AllocaInst * CreateAlloca(llvm::Function * function,
                                    std::string_view name,
                                    llvm::Value * init = nullptr);

    std::unique_ptr<llvm::LLVMContext> context;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::Module> module;
    std::map<std::string, llvm::AllocaInst *> named_values;
    std::unique_ptr<llvm::legacy::FunctionPassManager> fpm;

    struct Error
    {
        Error(const std::string & msg) : msg(msg) { throw 1; }
        std::string msg;
    };
    // Used to communicate the codegen result between different visited nodes
    std::variant<std::monostate, llvm::Function *, llvm::Value *, Error> result;

    const std::vector<std::unique_ptr<ast::Node>> & root;
};
}  // namespace mk

#endif