#include <gtest/gtest.h>

#include "compiler/lexer/lexer.h"
#include "compiler/parser/ast.h"
#include "compiler/parser/parser.h"
#include "compiler/parser/visitor.h"

#include "util/overload.h"


#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

TEST(Lexer, Simple)
{
    using namespace mk;

    const std::string code = R"CODE(
        1+1
        extern sin(arg);
        def foo()
            sin(1*1.22 < 2 * (1 + 42))

    )CODE";

    mk::Lexer lexer(code);

    std::vector<Token> expected = {{Double{1}},
                                   {Add{}},
                                   {Double{1}},
                                   {Extern{}},
                                   {Identifier{"sin"}},
                                   {Left{}},
                                   {Identifier{"arg"}},
                                   {Right{}},
                                   {SemiColon{}},
                                   {Def{}},
                                   {Identifier{"foo"}},
                                   {Left{}},
                                   {Right{}},
                                   {Identifier{"sin"}},
                                   {Left{}},
                                   {Double{1}},
                                   {Multiply{}},
                                   {Double{1.22}},
                                   {LessThan{}},
                                   {Double{2}},
                                   {Multiply{}},
                                   {Left{}},
                                   {Double{1}},
                                   {Add{}},
                                   {Double{42}},
                                   {Right{}},
                                   {Right{}}};

    std::vector<Token> actual;

    const auto overload = util::Overload([](const Empty &) { return true; },
                                         [&actual](const Def & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Extern & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Identifier & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Double & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Add & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Minus & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Multiply & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const LessThan & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Left & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Right & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const SemiColon & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Comma & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Invalid & t) {
                                             actual.emplace_back(t);
                                             return true;
                                         });

    do
    {
        lexer.next();
    } while (!std::visit(overload, lexer.current()));

    ASSERT_EQ(actual, expected);
}

class TestVisitor : public mk::ast::Visitor
{
private:
    std::stringstream & ss;
    void visit(mk::ast::Variable & variable) override { ss << variable.name; }

    void visit(mk::ast::Literal & literal) override { ss << literal.value; }

    void visit(mk::ast::BinExpr & bin_expr) override
    {
        std::string op;
        switch (bin_expr.op)
        {
        case mk::ast::BinExpr::Op::Add:
            op = "+";
            break;
        case mk::ast::BinExpr::Op::Minus:
            op = "-";
            break;
        case mk::ast::BinExpr::Op::Multiply:
            op = "*";
            break;
        case mk::ast::BinExpr::Op::LessThan:
            op = "<";
            break;
        }

        ss << "(";
        if (bin_expr.lhs)
            bin_expr.lhs->accept(*this);
        ss << op;
        if (bin_expr.rhs)
            bin_expr.rhs->accept(*this);
        ss << ")";
    }

    void visit(mk::ast::CallExpr & call_expr) override
    {
        ss << call_expr.name << "(";
        if (!call_expr.args.empty())
            if (call_expr.args.front())
                call_expr.args.front()->accept(*this);

        for (size_t i = 1; i < call_expr.args.size(); ++i)
            if (call_expr.args[i])
            {
                ss << ",";
                call_expr.args[i]->accept(*this);
            }

        ss << ")";
    }

    void visit(mk::ast::ProtoType & proto_type) override
    {
        ss << proto_type.name << "(";
        if (!proto_type.args.empty())
            ss << proto_type.args.front();
        for (size_t i = 1; i < proto_type.args.size(); ++i)
        {
            ss << "," << proto_type.args[i];
        }
        ss << ")";
    }

    void visit(mk::ast::Function & function) override
    {
        ss << "def ";
        function.prototype->accept(*this);
        ss << std::endl;
        function.body->accept(*this);
    }

    void visit(mk::ast::Error & error) override
    {
        ss << error.msg << std::endl;
    }

    void visit(mk::ast::Extern & e) override
    {
        ss << "extern ";
        e.prototype->accept(*this);
    }

public:
    TestVisitor(std::stringstream & ss) : ss(ss) {}
};


TEST(Parser, Simple)
{
    using namespace mk;

    const auto code = R"CODE(
        1+2
        extern bar(a,b)
        def foo(a, b)
            1 + (2*3) + 2 * 3 + 2 + bar(1,2)
    )CODE";

    std::stringstream ss;
    TestVisitor visitor(ss);

    Lexer lexer(code);
    Parser parser(lexer);
    const auto & nodes = parser.parse();

    for (auto & node : nodes)
    {
        if (node)
        {
            node->accept(visitor);
            ss << "\n";
        }
    }

    const std::string expected =
R"CODE((1+2)
extern bar(a,b)
def foo(a,b)
((((1+(2*3))+(2*3))+2)+bar(1,2))
)CODE";

    ASSERT_EQ(expected, ss.str());
}