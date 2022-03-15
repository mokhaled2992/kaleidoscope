#include "codegen.h"

#include "compiler/parser/ast.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include <string_view>

namespace mk
{
CodeGen::CodeGen(const std::vector<std::unique_ptr<ast::Node>> & root)
    : context(std::make_unique<llvm::LLVMContext>())
    , builder(std::make_unique<llvm::IRBuilder<>>(*context))
    , module(std::make_unique<llvm::Module>("my cool jit", *context))
    , root(root)
{}

void CodeGen::operator()()
{
    for (auto & node : root)
        if (node)
            node->accept(*this);
}

void CodeGen::visit(ast::Variable & variable)
{
    using namespace std::literals;

    if (const auto it = named_values.find(variable.name);
        it == named_values.cend())
    {
        result = Error{std::string("Unknown symbol") + variable.name};
    }
    else
    {
        result = it->second;
    }
}

void CodeGen::visit(ast::Literal & literal)
{
    result = llvm::ConstantFP::get(*context, llvm::APFloat(literal.value));
}

void CodeGen::visit(ast::BinExpr & bin_expr)
{
    if (bin_expr.lhs)
        bin_expr.lhs->accept(*this);
    auto l = std::get_if<llvm::Value *>(&result);
    if (bin_expr.rhs)
        bin_expr.rhs->accept(*this);
    auto r = std::get_if<llvm::Value *>(&result);
    if (!l || !r)
    {
        result = Error{"bad binary expression"};
        return;
    }

    switch (bin_expr.op)
    {
    case ast::BinExpr::Op::Add:
        result = builder->CreateFAdd(*l, *r, "addtmp");
        break;
    case ast::BinExpr::Op::Minus:
        result = builder->CreateFSub(*l, *r, "subtmp");
        break;
    case ast::BinExpr::Op::Multiply:
        result = builder->CreateFMul(*l, *r, "multmp");
        break;
    case ast::BinExpr::Op::LessThan:
    {
        const auto boolean = builder->CreateFCmpULT(*l, *r, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        result = builder->CreateUIToFP(boolean,
                                       llvm::Type::getDoubleTy(*context),
                                       "booltmp");
    }
    break;
    default:
        result = Error{"invalid binary operator"};
    }
}

void CodeGen::visit(ast::CallExpr & call_expr)
{
    auto callee = module->getFunction(call_expr.name);
    if (!callee)
    {
        result = Error{"Unknown function referenced"};
        return;
    }

    if (callee->arg_size() != call_expr.args.size())
    {
        result = Error{
            "Mismatch in the number of arguments between the function call and "
            "definition"};
        return;
    }

    std::vector<llvm::Value *> args;

    for (const auto & arg : call_expr.args)
    {
        if (arg)
        {
            arg->accept(*this);
            auto a = std::get_if<llvm::Value *>(&result);
            if (!a || !*a)
                goto err;
            args.push_back(*a);
        }
        else
        {
        err:
            result = Error{"bad argument"};
            return;
        }
    }

    result = builder->CreateCall(callee, std::move(args), "calltmp");
}

void CodeGen::visit(ast::ProtoType & prototype)
{
    auto signature = llvm::FunctionType::get(
        llvm::Type::getDoubleTy(*context),
        std::vector<llvm::Type *>(prototype.args.size(),
                                  llvm::Type::getDoubleTy(*context)),
        false);

    auto function = llvm::Function::Create(signature,
                                           llvm::Function::ExternalLinkage,
                                           prototype.name,
                                           module.get());

    size_t i = 0;
    for (auto & arg : function->args())
    {
        arg.setName(prototype.args[i++]);
    }

    result = function;
}

void CodeGen::visit(ast::Function & fun)
{
    if (fun.prototype)
    {
        auto function = module->getFunction(fun.prototype->name);
        if (!function)
        {
            fun.prototype->accept(*this);
            if (auto p = std::get_if<llvm::Function *>(&result))
                function = *p;
        }

        if (!function)
            goto err_signature;

        if (!function->empty())
        {
            result = Error{"function is already defined"};
            return;
        }

        auto * bb = llvm::BasicBlock::Create(*context, "entry", function);
        builder->SetInsertPoint(bb);

        named_values.clear();
        for (auto & arg : function->args())
            named_values.emplace(arg.getName(), &arg);

        if (fun.body)
        {
            fun.body->accept(*this);
            auto ret = std::get_if<llvm::Value *>(&result);
            if (!ret || !*ret)
            {
                function->eraseFromParent();
                goto err_body;
            }
            builder->CreateRet(*ret);
            llvm::verifyFunction(*function);

            result = function;
        }
        else
        {
        err_body:
            result = Error{"bad function body"};
            return;
        }
    }
    else
    {
    err_signature:
        result = Error{"bad function signature"};
    }
}

void CodeGen::visit(ast::Extern & e)
{
    if (e.prototype)
        e.prototype->accept(*this);
}

void CodeGen::visit(ast::Error & e)
{
    result = Error{e.msg};
}

}  // namespace mk