#ifndef __AST_H__
#define __AST_H__

#include "visitor.h"

#include <memory>
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

class BinExpr : public Expr
{
public:
    enum class Op : std::uint8_t
    {
        Add,
        Multiply,
        Minus,
        LessThan
    };

    inline static const std::unordered_map<Op, std::int64_t> precedence = {
        {Op::LessThan, 10},
        {Op::Add, 20},
        {Op::Minus, 20},
        {Op::Multiply, 40},
    };

    BinExpr(Op && op,
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

    Op op;
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