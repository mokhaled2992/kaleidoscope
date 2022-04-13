#include "codegen.h"

#include "compiler/parser/ast.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"

#include <string_view>


namespace mk
{
CodeGen::CodeGen(const std::vector<std::unique_ptr<ast::Node>> & root)
    : context(std::make_unique<llvm::LLVMContext>())
    , builder(std::make_unique<llvm::IRBuilder<>>(*context))
    , module(std::make_unique<llvm::Module>("my cool jit", *context))
    , fpm(std::make_unique<llvm::legacy::FunctionPassManager>(module.get()))
    , root(root)
{
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    fpm->add(llvm::createInstructionCombiningPass());
    // Reassociate expressions.
    fpm->add(llvm::createReassociatePass());
    // Eliminate Common SubExpressions.
    fpm->add(llvm::createGVNPass());
    // Simplify the control flow graph (deleting unreachable blocks, etc).
    fpm->add(llvm::createCFGSimplificationPass());

    fpm->doInitialization();
}

CodeGen::~CodeGen() = default;

const llvm::Module * CodeGen::operator()()
{
    for (auto & node : root)
        if (node)
            node->accept(*this);
    return module.get();
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
    llvm::Value *l = nullptr, *r = nullptr;
    if (bin_expr.lhs)
    {
        result = std::monostate{};
        bin_expr.lhs->accept(*this);

        if (auto p = std::get_if<llvm::Value *>(&result))
            l = *p;
    }

    if (bin_expr.rhs)
    {
        result = std::monostate{};
        bin_expr.rhs->accept(*this);
        if (auto p = std::get_if<llvm::Value *>(&result))
            r = *p;
    }
    if (!l || !r)
    {
        result = Error("bad binary expression");
        return;
    }

    switch (bin_expr.op.front())
    {
    case '+':
    {
        result = builder->CreateFAdd(l, r, "addtmp");
        break;
    }
    case '-':
    {
        result = builder->CreateFSub(l, r, "subtmp");
        break;
    }
    case '*':
    {
        result = builder->CreateFMul(l, r, "multmp");
        break;
    }
    case '<':
    {
        const auto boolean = builder->CreateFCmpULT(l, r, "cmptmp");
        // Convert bool 0/1 to double 0.0 or 1.0
        result = builder->CreateUIToFP(boolean,
                                       llvm::Type::getDoubleTy(*context),
                                       "booltmp");
        break;
    }
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
            result = std::monostate{};
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
            result = std::monostate{};
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
            result = std::monostate{};
            fun.body->accept(*this);
            auto ret = std::get_if<llvm::Value *>(&result);
            if (!ret || !*ret)
            {
                function->eraseFromParent();
                goto err_body;
            }
            builder->CreateRet(*ret);
            llvm::verifyFunction(*function);

            fpm->run(*function);

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

void CodeGen::visit(ast::ConditionalExpr & conditional)
{
    if (conditional.condition)
    {
        result = std::monostate{};
        conditional.condition->accept(*this);
        if (auto p = std::get_if<llvm::Value *>(&result))
        {
            auto condition_value = builder->CreateFCmpONE(
                *p,
                llvm::ConstantFP::get(*context, llvm::APFloat(0.0)),
                "ifcond");
            auto function = builder->GetInsertBlock()->getParent();

            auto * first_block =
                llvm::BasicBlock::Create(*context, "then", function);
            auto * second_block = llvm::BasicBlock::Create(*context, "else");
            auto * third_block = llvm::BasicBlock::Create(*context, "ifcont");

            builder->CreateCondBr(condition_value, first_block, second_block);

            if (conditional.first)
            {
                builder->SetInsertPoint(first_block);
                result = std::monostate{};
                conditional.first->accept(*this);
                if (auto p = std::get_if<llvm::Value *>(&result))
                {
                    auto first_value = *p;
                    builder->CreateBr(third_block);
                    first_block = builder->GetInsertBlock();

                    if (conditional.second)
                    {
                        function->getBasicBlockList().push_back(second_block);
                        builder->SetInsertPoint(second_block);
                        result = std::monostate{};
                        conditional.second->accept(*this);
                        if (auto p = std::get_if<llvm::Value *>(&result))
                        {
                            auto second_value = *p;
                            builder->CreateBr(third_block);
                            second_block = builder->GetInsertBlock();

                            function->getBasicBlockList().push_back(
                                third_block);
                            builder->SetInsertPoint(third_block);

                            auto * phi_node = builder->CreatePHI(
                                llvm::Type::getDoubleTy(*context), 2, "iftmp");
                            phi_node->addIncoming(first_value, first_block);
                            phi_node->addIncoming(second_value, second_block);
                            result = phi_node;
                            return;
                        }
                    }
                }
            }
        }
    }
    result = std::monostate{};
}

void CodeGen::visit(ast::ForExpr & f)
{
    result = std::monostate{};
    f.init->accept(*this);
    if (auto p = std::get_if<llvm::Value *>(&result))
    {
        auto init = *p;

        auto function = builder->GetInsertBlock()->getParent();
        auto loop = llvm::BasicBlock::Create(*context, "loop", function);
        builder->CreateBr(loop);

        auto before = builder->GetInsertBlock();

        builder->SetInsertPoint(loop);

        auto phi_node =
            builder->CreatePHI(llvm::Type::getDoubleTy(*context), 2, f.name);

        phi_node->addIncoming(init, before);

        llvm::Value * old = nullptr;
        if (auto it = named_values.find(f.name); it != named_values.cend())
        {
            old = it->second;
            it->second = phi_node;
        }
        else
        {
            named_values.emplace(f.name, phi_node);
        }


        result = std::monostate{};
        f.body->accept(*this);


        llvm::Value * next = nullptr;
        if (f.step)
        {
            result = std::monostate{};
            f.step->accept(*this);
            if (const auto p = std::get_if<llvm::Value *>(&result))
            {
                next = builder->CreateFAdd(phi_node, *p, "next");
            }
        }
        else
        {
            next =
                builder->CreateFAdd(phi_node,
                                    llvm::ConstantFP::get(*context,
                                                          llvm::APFloat(1.0)),
                                    "next");
        }

        result = std::monostate{};
        f.condition->accept(*this);
        if (const auto p = std::get_if<llvm::Value *>(&result))
        {
            auto condition = builder->CreateFCmpONE(
                *p,
                llvm::ConstantFP::get(*context, llvm::APFloat(0.0)),
                "condition");

            auto loop_end = builder->GetInsertBlock();
            phi_node->addIncoming(next, loop_end);

            auto after = llvm::BasicBlock::Create(*context, "after", function);
            builder->CreateCondBr(condition, loop, after);

            builder->SetInsertPoint(after);
            if (old)
                named_values[f.name] = old;
            else
                named_values.erase(f.name);
        }

        result =
            llvm::Constant::getNullValue(llvm::Type::getDoubleTy(*context));
    }
}

}  // namespace mk