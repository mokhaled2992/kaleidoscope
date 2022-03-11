#ifndef __AST_H__
#define __AST_H__

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace mk
{
namespace ast
{
class Node
{
public:
    virtual ~Node() = default;
};

class Expr : public Node
{
};

class Variable : public Expr
{
public:
    Variable(std::string && name) : name(std::move(name)) {}

private:
    std::string name;
};

class Literal : public Expr
{
public:
    Literal(double value) : value(value) {}

private:
    double value;
};

class BinExpr : public Expr
{
public:
    enum class Op
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

private:
    Op op;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

class CallExpr : public Expr
{
public:
    CallExpr(std::string && name, std::vector<std::unique_ptr<Expr>> && args)
        : name(std::move(name)), args(std::move(args))
    {}

private:
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

private:
    std::string name;
    std::vector<std::string> args;
};

class Function : public ProtoType
{
public:
    Function(ProtoType && prototype, std::unique_ptr<Expr> && body)
        : ProtoType(std::move(prototype)), body(std::move(body))
    {}

private:
    std::unique_ptr<Expr> body;
};
}  // namespace ast
}  // namespace mk

#endif