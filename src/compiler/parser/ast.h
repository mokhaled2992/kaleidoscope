#ifndef __AST_H__
#define __AST_H__

#include "visitor.h"

#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mk
{
namespace ast
{
class Visitor;

class Node
{
public:
    virtual ~Node() = default;
    virtual void accept(Visitor & visitor) = 0;
    virtual void accept_children(Visitor & visitor) = 0;
};

class Expr : public Node
{
public:
    ~Expr() override = default;
};

class Variable : public Expr
{
public:
    Variable(std::string && name) : name(std::move(name)) {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override {}
    std::string name;
};

class Literal : public Expr
{
public:
    Literal(double value) : value(value) {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override {}

    double value;
};

class LetExpr : public Expr
{
public:
    LetExpr(std::vector<std::pair<std::string, std::unique_ptr<Expr>>> && vars,
            std::unique_ptr<Expr> && body)
        : vars(std::move(vars)), body(std::move(body))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }

    void accept_children(Visitor & visitor) override
    {
        for (auto & [name, value] : vars)
            if (value)
                value->accept(visitor);
        if (body)
            body->accept(visitor);
    }

    std::vector<std::pair<std::string, std::unique_ptr<Expr>>> vars;
    std::unique_ptr<Expr> body;
};

class UnaryExpr : public Expr
{
public:
    UnaryExpr(std::string && op, std::unique_ptr<Expr> && operand)
        : op(std::move(op)), operand(std::move(operand))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }

    void accept_children(Visitor & visitor) override
    {
        if (operand)
            operand->accept(visitor);
    }

    std::string op;
    std::unique_ptr<Expr> operand;
};

class BinExpr : public Expr
{
public:
    BinExpr(std::string && op,
            std::unique_ptr<Expr> && lhs,
            std::unique_ptr<Expr> && rhs)
        : op(std::move(op)), lhs(std::move(lhs)), rhs(std::move(rhs))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override
    {
        if (lhs)
            lhs->accept(visitor);
        if (rhs)
            rhs->accept(visitor);
    }

    std::string op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

class ConditionalExpr : public Expr
{
public:
    ConditionalExpr(std::unique_ptr<Expr> && condition,
                    std::unique_ptr<Expr> && first,
                    std::unique_ptr<Expr> && second)
        : condition(std::move(condition))
        , first(std::move(first))
        , second(std::move(second))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }

    void accept_children(Visitor & visitor) override
    {
        if (condition)
            condition->accept(visitor);
        if (first)
            first->accept(visitor);
        if (second)
            second->accept(visitor);
    }

    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> first;
    std::unique_ptr<Expr> second;
};

class ForExpr : public Expr
{
public:
    ForExpr(std::string && name,
            std::unique_ptr<Expr> && init,
            std::unique_ptr<Expr> && condition,
            std::unique_ptr<Expr> && step,
            std::unique_ptr<Expr> && body)
        : name(std::move(name))
        , init(std::move(init))
        , condition(std::move(condition))
        , step(std::move(step))
        , body(std::move(body))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }

    void accept_children(Visitor & visitor) override
    {
        if (init)
            init->accept(visitor);
        if (condition)
            condition->accept(visitor);
        if (step)
            step->accept(visitor);
        if (body)
            body->accept(visitor);
    }

    std::string name;
    std::unique_ptr<Expr> init;
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Expr> step;
    std::unique_ptr<Expr> body;
};

class CallExpr : public Expr
{
public:
    CallExpr(std::string && name, std::vector<std::unique_ptr<Expr>> && args)
        : name(std::move(name)), args(std::move(args))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override
    {

        for (auto & arg : args)
        {
            if (arg)
                arg->accept(visitor);
        }
    }

    std::string name;
    std::vector<std::unique_ptr<Expr>> args;
};

class ProtoType : public Node
{
public:
    ProtoType(std::string && name, std::vector<std::string> && args)
        : name(std::move(name)), args(std::move(args))
    {}

    ProtoType(ProtoType &&) = default;

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override {}

    std::string name;
    std::vector<std::string> args;
};

class Extern : public Node
{
public:
    Extern(std::unique_ptr<ProtoType> && prototype)
        : prototype(std::move(prototype))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override
    {
        if (prototype)
            prototype->accept(visitor);
    }
    std::unique_ptr<ast::ProtoType> prototype;
};


class Function : public Node
{
public:
    Function(std::unique_ptr<ProtoType> && prototype,
             std::unique_ptr<Expr> && body)
        : prototype(std::move(prototype)), body(std::move(body))
    {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override
    {
        if (prototype)
            prototype->accept(visitor);
        if (body)
            body->accept(visitor);
    }
    std::unique_ptr<ProtoType> prototype;
    std::unique_ptr<Expr> body;
};

class Error : public Node
{
public:
    Error(const std::string & msg) : msg(msg) {}

    void accept(Visitor & visitor) override { visitor.visit(*this); }
    void accept_children(Visitor & visitor) override {}

    std::string msg;
};

}  // namespace ast
}  // namespace mk

#endif