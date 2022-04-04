#ifndef __VISITOR_H__
#define __VISITOR_H__

namespace mk
{
namespace ast
{

class Variable;
class Literal;
class BinExpr;
class CallExpr;
class ConditionalExpr;
class ProtoType;
class Function;
class Error;
class Extern;

class Visitor
{
public:
    virtual ~Visitor() = default;

    virtual void visit(Variable &) = 0;
    virtual void visit(Literal &) = 0;
    virtual void visit(BinExpr &) = 0;
    virtual void visit(ConditionalExpr &) = 0;
    virtual void visit(CallExpr &) = 0;
    virtual void visit(ProtoType &) = 0;
    virtual void visit(Function &) = 0;
    virtual void visit(Extern &) = 0;
    virtual void visit(Error &) = 0;
};
}  // namespace ast
}  // namespace mk

#endif