#include <gtest/gtest.h>

#include "compiler/codegen/codegen.h"
#include "compiler/driver/driver.h"
#include "compiler/lexer/lexer.h"
#include "compiler/parser/ast.h"
#include "compiler/parser/parser.h"
#include "compiler/parser/visitor.h"

#include "util/overload.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"


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
            sin(1*1.22 < 2 * (1 + 42)) + if(1+2) then 3 else 3.14 + for i=1, i<10, 1.0 in sin(1)

    )CODE";

    mk::Lexer lexer(code);

    std::vector<Token> expected = {
        {Double{1}},
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
        {Right{}},
        {Add{}},
        {If{}},
        {Left{}},
        {Double{1}},
        {Add{}},
        {Double{2}},
        {Right{}},
        {Then{}},
        {Double{3}},
        {Else{}},
        {Double{3.14}},
        {Add{}},
        {For{}},
        {Identifier{"i"}},
        {Assignment{}},
        {Double{1}},
        {Comma{}},
        {Identifier{"i"}},
        {LessThan{}},
        {Double{10}},
        {Comma{}},
        {Double{1.0}},
        {In{}},
        {Identifier{"sin"}},
        {Left{}},
        {Double{1}},
        {Right{}},
    };

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
                                         },
                                         [&actual](const If & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Then & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Else & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const For & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const In & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Assignment & t) {
                                             actual.emplace_back(t);
                                             return false;
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

    void visit(mk::ast::ConditionalExpr & conditional) override
    {
        ss << "if(";
        conditional.condition->accept(*this);
        ss << ")"
           << "\n"
           << "then"
           << "\n";
        conditional.first->accept(*this);
        ss << "\n"
           << "else"
           << "\n";
        conditional.second->accept(*this);
    }

    void visit(mk::ast::ForExpr & f) override
    {
        ss << "for " << f.name << "=";
        f.init->accept(*this);
        ss << ", ";
        f.condition->accept(*this);
        if (f.step)
        {
            ss << ", ";
            f.step->accept(*this);
        }
        ss << " in" << std::endl;
        f.body->accept(*this);
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
            1 + (2*3) + 2 * 3 + 2 + bar(1,2) + for i=1, i<10, 2 in if(a * b) then bar(1,3) else foo(4,5) + 1 * 3
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
(((((1+(2*3))+(2*3))+2)+bar(1,2))+for i=1, (i<10), 2 in
if((a*b))
then
bar(1,3)
else
(foo(4,5)+(1*3)))
)CODE";

    ASSERT_EQ(expected, ss.str());
}

TEST(CodeGen, Simple)
{
    using namespace mk;

    const auto code = R"CODE(
        extern bar(a,b)
        def foo(a, b)
            1 + (2*3+a) + 4 * 5 + 6 + bar(7,8) * b + if(bar(a,b) < b) then a*b else a + b * bar(13,14)
        def main()
            foo(9,10)
    )CODE";

    Lexer lexer(code);
    Parser parser(lexer);

    CodeGen codegen(parser.parse());

    auto module = codegen();

    std::error_code error;

    std::string actual;
    {
        llvm::raw_string_ostream ss(actual);
        module->print(ss, nullptr);
    }

    const std::string expected =
        R"CODE(; ModuleID = 'my cool jit'
source_filename = "my cool jit"

declare double @bar(double, double)

define double @foo(double %a, double %b) {
entry:
  %calltmp = call double @bar(double 7.000000e+00, double 8.000000e+00)
  %calltmp5 = call double @bar(double %a, double %b)
  %cmptmp = fcmp ult double %calltmp5, %b
  br i1 %cmptmp, label %then, label %else

then:                                             ; preds = %entry
  %multmp6 = fmul double %a, %b
  br label %ifcont

else:                                             ; preds = %entry
  %calltmp7 = call double @bar(double 1.300000e+01, double 1.400000e+01)
  %multmp8 = fmul double %b, %calltmp7
  %addtmp9 = fadd double %a, %multmp8
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %iftmp = phi double [ %multmp6, %then ], [ %addtmp9, %else ]
  %addtmp = fadd double %a, 6.000000e+00
  %addtmp1 = fadd double %addtmp, 1.000000e+00
  %addtmp2 = fadd double %addtmp1, 2.000000e+01
  %addtmp3 = fadd double %addtmp2, 6.000000e+00
  %multmp = fmul double %b, %calltmp
  %addtmp4 = fadd double %addtmp3, %multmp
  %addtmp10 = fadd double %addtmp4, %iftmp
  ret double %addtmp10
}

define double @main() {
entry:
  %calltmp = call double @foo(double 9.000000e+00, double 1.000000e+01)
  ret double %calltmp
}
)CODE";

    ASSERT_EQ(expected, actual);
}

TEST(driver, execute)
{
    const std::string code = R"CODE(
        def foo(a, b)
            1 + (2*3+a) + 4 * 5 + 6 * b * if(a<b)then 16*b else 32*a
        def main()
            foo(9,10)
    )CODE";

    mk::Driver driver;

    mk::Driver::Action execute(mk::Driver::Execute{});
    driver(code, execute);
    std::visit(mk::util::Overload([](double x) { ASSERT_EQ(x, 9636); },
                                  [](...) { FAIL(); }),
               std::get<mk::Driver::Execute>(execute).result);
}

TEST(driver, link)
{
    const std::string code = R"CODE(
        extern bar(a,b)
        def foo(a, b)
            1 + (2*3+a) + 4 * 5 + 6 * b
        def main()
            foo(9,10)
    )CODE";

    mk::Driver driver;

    mk::Driver::Action link(mk::Driver::Link{});

    driver(code, link);

    ASSERT_TRUE(std::get<mk::Driver::Link>(link).error);
}