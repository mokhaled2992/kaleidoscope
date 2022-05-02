#include <gtest/gtest.h>

#include "compiler/codegen/codegen.h"
#include "compiler/driver/driver.h"
#include "compiler/lexer/lexer.h"
#include "compiler/lexer/token.h"
#include "compiler/parser/ast.h"
#include "compiler/parser/parser.h"
#include "compiler/parser/visitor.h"

#include "util/lld.h"
#include "util/overload.h"

#include "lld/Common/Driver.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/raw_ostream.h"

#include "Poco/Process.h"

#include "fmt/core.h"

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
            let
                x = 10
            in
                sin(1*1.22 < 2 * (1 + 42)) + if(1+2) then 3 else 3.14 + for i=1, i<10, 1.0 in sin(1)
        def operator | 10(l,r)
            1
        def operator!(l)
            1
        def operator&10(l,r)
            1

    )CODE";

    mk::Lexer lexer(code);

    std::vector<Token> expected = {
        {1.0},
        {static_cast<unsigned char>('+')},
        {1.0},
        {Extern{}},
        {Identifier{"sin"}},
        {static_cast<unsigned char>('(')},
        {Identifier{"arg"}},
        {static_cast<unsigned char>(')')},
        {static_cast<unsigned char>(';')},
        {Def{}},
        {Identifier{"foo"}},
        {static_cast<unsigned char>('(')},
        {static_cast<unsigned char>(')')},
        {Let{}},
        {Identifier{"x"}},
        {static_cast<unsigned char>('=')},
        {10.0},
        {In{}},
        {Identifier{"sin"}},
        {static_cast<unsigned char>('(')},
        {1.0},
        {static_cast<unsigned char>('*')},
        {1.22},
        {static_cast<unsigned char>('<')},
        {2.0},
        {static_cast<unsigned char>('*')},
        {static_cast<unsigned char>('(')},
        {1.0},
        {static_cast<unsigned char>('+')},
        {42.0},
        {static_cast<unsigned char>(')')},
        {static_cast<unsigned char>(')')},
        {static_cast<unsigned char>('+')},
        {If{}},
        {static_cast<unsigned char>('(')},
        {1.0},
        {static_cast<unsigned char>('+')},
        {2.0},
        {static_cast<unsigned char>(')')},
        {Then{}},
        {3.0},
        {Else{}},
        {3.14},
        {static_cast<unsigned char>('+')},
        {For{}},
        {Identifier{"i"}},
        {static_cast<unsigned char>('=')},
        {1.0},
        {static_cast<unsigned char>(',')},
        {Identifier{"i"}},
        {static_cast<unsigned char>('<')},
        {10.0},
        {static_cast<unsigned char>(',')},
        {1.0},
        {In{}},
        {Identifier{"sin"}},
        {static_cast<unsigned char>('(')},
        {1.0},
        {static_cast<unsigned char>(')')},
        {Def{}},
        {Operator{"|"}},
        {10.0},
        {static_cast<unsigned char>('(')},
        {Identifier{"l"}},
        {static_cast<unsigned char>(',')},
        {Identifier{"r"}},
        {static_cast<unsigned char>(')')},
        {1.0},
        {Def{}},
        {Operator{"!"}},
        {static_cast<unsigned char>('(')},
        {Identifier{"l"}},
        {static_cast<unsigned char>(')')},
        {1.0},
        {Def{}},
        {Operator{"&"}},
        {10.0},
        {static_cast<unsigned char>('(')},
        {Identifier{"l"}},
        {static_cast<unsigned char>(',')},
        {Identifier{"r"}},
        {static_cast<unsigned char>(')')},
        {1.0},
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
                                         [&actual](const double & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const unsigned char & t) {
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
                                         [&actual](const Operator & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         },
                                         [&actual](const Let & t) {
                                             actual.emplace_back(t);
                                             return false;
                                         });

    do
    {
        lexer.next();
    } while (
        !std::visit(overload, static_cast<const TokenType &>(lexer.current())));

    ASSERT_EQ(actual, expected);
}

class TestVisitor : public mk::ast::Visitor
{
private:
    std::stringstream & ss;
    void visit(mk::ast::Variable & variable) override { ss << variable.name; }

    void visit(mk::ast::Literal & literal) override { ss << literal.value; }

    void visit(mk::ast::UnaryExpr & unary_expr) override
    {
        ss << unary_expr.op << "";
        if (unary_expr.operand)
            unary_expr.operand->accept(*this);
    }

    void visit(mk::ast::BinExpr & bin_expr) override
    {
        ss << "(";
        if (bin_expr.lhs)
            bin_expr.lhs->accept(*this);
        ss << bin_expr.op;
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

    void visit(mk::ast::LetExpr & let) override
    {
        ss << "let" << std::endl;
        for (auto & [name, expr] : let.vars)
        {
            ss << name << "=";
            expr->accept(*this);
            ss << std::endl;
        }
        ss << "in" << std::endl;
        let.body->accept(*this);
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
        def operator!(l)
            if(l) then 0 else 1
        def operator&100(l,r)
            0
        def foo(a, b)
            let
                x = 1
                y = 2
            in
                x + y + 1 + (2*3) + 2 * 3 + 2 + !bar(1,2) * !a & !b + for i=1, i<10, 2 in if(a * b) then bar(1,3) else foo(4,5) + 1 * 3
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
def !(l)
if(l)
then
0
else
1
def &(l,r)
0
def foo(a,b)
let
x=1
y=2
in
(((((((x+y)+1)+(2*3))+(2*3))+2)+(!bar(1,2)*(!a&!b)))+for i=1, (i<10), 2 in
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
        extern operator&100(l,r)
        extern operator!(l)
        def foo(a, b)
            1 + (2*3+a) + 4 * 5 + 6 + bar(7,8) * b & a + (if(!bar(a,b) < b) then a*b else a + b * bar(13,14)) + for i=0,i<10,2 in bar(a,b)
        def main()
            foo(9,10)
    )CODE";

    Lexer lexer(code);
    Parser parser(lexer);

    CodeGen codegen(parser.parse());

    auto module = codegen();

    std::string actual;
    {
        llvm::raw_string_ostream ss(actual);
        module->print(ss, nullptr);
    }

    const std::string expected =
        R"CODE(; ModuleID = 'my cool jit'
source_filename = "my cool jit"

declare double @bar(double, double)

declare double @"&"(double, double)

declare double @"!"(double)

define double @foo(double %a, double %b) {
entry:
  %calltmp = call double @bar(double 7.000000e+00, double 8.000000e+00)
  %"&" = call double @"&"(double %b, double %a)
  %calltmp12 = call double @bar(double %a, double %b)
  %"!" = call double @"!"(double %calltmp12)
  %cmptmp = fcmp ult double %"!", %b
  br i1 %cmptmp, label %then, label %else

then:                                             ; preds = %entry
  %multmp16 = fmul double %a, %b
  br label %ifcont

else:                                             ; preds = %entry
  %calltmp19 = call double @bar(double 1.300000e+01, double 1.400000e+01)
  %multmp20 = fmul double %b, %calltmp19
  %addtmp21 = fadd double %a, %multmp20
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %iftmp = phi double [ %multmp16, %then ], [ %addtmp21, %else ]
  br label %loop

loop:                                             ; preds = %loop, %ifcont
  %i.0 = phi double [ 0.000000e+00, %ifcont ], [ %next, %loop ]
  %calltmp25 = call double @bar(double %a, double %b)
  %next = fadd double %i.0, 2.000000e+00
  %cmptmp28 = fcmp ult double %i.0, 1.000000e+01
  br i1 %cmptmp28, label %loop, label %after

after:                                            ; preds = %loop
  %addtmp = fadd double %a, 6.000000e+00
  %addtmp4 = fadd double %addtmp, 1.000000e+00
  %addtmp5 = fadd double %addtmp4, 2.000000e+01
  %addtmp6 = fadd double %addtmp5, 6.000000e+00
  %multmp = fmul double %calltmp, %"&"
  %addtmp9 = fadd double %addtmp6, %multmp
  %addtmp22 = fadd double %addtmp9, %iftmp
  %addtmp30 = fadd double %addtmp22, 0.000000e+00
  ret double %addtmp30
}

define i32 @main() {
entry:
  %calltmp = call double @foo(double 9.000000e+00, double 1.000000e+01)
  %status = fptosi double %calltmp to i32
  ret i32 %status
}
)CODE";

    ASSERT_EQ(expected, actual);
}

TEST(driver, execute)
{
    const std::string code = R"CODE(
        def operator!(l)
            0-l
        def operator&100(l,r)
            if(l) then if(r) then 1 else 0 else 0
        def foo(a, b)
            2*6&1 + !1 + (2*3+a) + 4 * 5 + 6 * b * if(a<b)then 16*b else 32*a
        def operator:1(l,r) r
        def baz()
            let
                x = 0
            in
                (for i = 1,i<10,1 in x = x + i) : x

        def main()
            foo(9,10) + baz()
    )CODE";

    mk::Driver driver;

    std::visit(mk::util::Overload(
                   [](int32_t x) {
                       ASSERT_EQ(x,
                                 9636 + 1 + 2 + 3 + 4 + 5 + 6 + 7 + 8 + 9 + 10);
                   },
                   [](...) { FAIL(); }),
               driver(code, mk::Driver::Execute{}));
}

TEST(driver, link)
{
    using namespace std::literals;
    using namespace mk;

    const std::string_view code1 = R"CODE(
        def bar(a,b) a
    )CODE";

    const std::string_view code2 = R"CODE(
        extern bar(a,b)
        extern baz(n)
        def foo(a, b)
            1 + (2*3+a) + 4 * 5 + 6 * b
        def main()
            foo(9,10) + baz(10)
    )CODE";


    Driver driver;

    const auto [_, module] = driver(std::vector{code2, code1}, Driver::Link{});

    const auto baz = module->getFunction("baz");
    ASSERT_TRUE(baz);
    ASSERT_TRUE(baz->empty());

    const auto bar = module->getFunction("bar");
    ASSERT_TRUE(bar);
    ASSERT_FALSE(bar->empty());

    const auto foo = module->getFunction("foo");
    ASSERT_TRUE(foo);
    ASSERT_FALSE(foo->empty());

    const auto main = module->getFunction("main");
    ASSERT_TRUE(main);
    ASSERT_FALSE(bar->empty());
}

// TEST(driver, object)
// {
//     using namespace std::literals;
//     using namespace mk;

//     const std::string_view code = R"CODE(
//         extern bar(a,b)
//         def foo(a, b)
//             1 + (2*3+a) + 4 * 5 + 6 * b + bar(a,a)
//         def main()
//             foo(4,3)
//     )CODE";

//     Driver driver;
//     Driver::Object::Args args{.outfile = "output"};
//     driver(code, args);

//     mk::lld::elf::ScopedLink{}({"ld",
//                                 "-dynamic-linker=/lib64/ld-linux-x86-64.so.2",
//                                 fmt::format("-o{}.out", args.outfile),
//                                 "-L/lib/x86_64-linux-gnu/",
//                                 "-Ltest/lib/",
//                                 fmt::format("{}.o", args.outfile),
//                                 "-lmylib",
//                                 "/lib/x86_64-linux-gnu/crt1.o",
//                                 "-lc"});

//     ASSERT_EQ(
//         Poco::Process::launch(fmt::format("./{}.out", args.outfile), {}).wait(),
//         57);
// }

// TEST(driver, shared_library)
// {
//     using namespace std::literals;
//     using namespace mk;

//     const std::string_view code = R"CODE(
//         extern bar(a,b)
//         def foo(a, b)
//             1 + (2*3+a) + 4 * 5 + 6 * b + 42 + 13 + bar(a,b)
//         def main()
//             foo(4,3)
//     )CODE";

//     Driver driver;

//     Driver::Library::Shared::Args args;
//     args.dynamic_linker = "/lib64/ld-linux-x86-64.so.2",
//     args.outfile = "output";

//     ASSERT_TRUE(driver(code, args));

//     mk::lld::elf::ScopedLink{}({"ld",
//                                 "-dynamic-linker=/lib64/ld-linux-x86-64.so.2",
//                                 fmt::format("-o{}.out", args.outfile),
//                                 "-L/lib/x86_64-linux-gnu/",
//                                 "-Ltest/lib/",
//                                 fmt::format("{}.so", args.outfile),
//                                 "-lmylib",
//                                 "/lib/x86_64-linux-gnu/crt1.o",
//                                 "-lc",
//                                 "-rpath=."});

//     ASSERT_EQ(
//         Poco::Process::launch(fmt::format("./{}.out", args.outfile), {}).wait(),
//         111);
// }

// TEST(driver, bitcode)
// {
//     using namespace std::literals;
//     using namespace mk;

//     const std::string_view code = R"CODE(
//         extern bar(a,b)
//         def foo(a, b)
//             1 + (2*3+a) + 4 * 5 + 6 * b + 42 - b - bar(a,b)
//         def main()
//             foo(4,3)
//     )CODE";

//     Driver driver;
//     Driver::Bitcode::Args args;
//     args.outfile = "output";
//     driver(code, args);

//     ASSERT_EQ(Poco::Process::launch("toolstack/bin/llc",
//                                     {"-filetype=obj",
//                                      fmt::format("{}.bc", args.outfile),
//                                      "-o",
//                                      fmt::format("{}.o", args.outfile)})
//                   .wait(),
//               0);

//     mk::lld::elf::ScopedLink{}({"ld",
//                                 "-dynamic-linker=/lib64/ld-linux-x86-64.so.2",
//                                 fmt::format("-o{}.out", args.outfile),
//                                 "-L/lib/x86_64-linux-gnu/",
//                                 "-Ltest/lib/",
//                                 fmt::format("{}.o", args.outfile),
//                                 "-lmylib",
//                                 "/lib/x86_64-linux-gnu/crt1.o",
//                                 "-lc",
//                                 "-rpath=."});

//     ASSERT_EQ(
//         Poco::Process::launch(fmt::format("./{}.out", args.outfile), {}).wait(),
//         81);
// }

// TEST(driver, ir)
// {
//     using namespace std::literals;
//     using namespace mk;

//     const std::string_view code = R"CODE(
//         extern bar(a,b)
//         def foo(a, b)
//             1 + (2*3+a) + 4 * 5 + 6 * b + 42 - b - bar(b,b)
//         def main()
//             foo(4,3)
//     )CODE";

//     Driver driver;
//     Driver::IR::Args args;
//     args.outfile = "output";
//     driver(code, args);

//     ASSERT_EQ(Poco::Process::launch("toolstack/bin/llc",
//                                     {"-filetype=obj",
//                                      fmt::format("{}.ll", args.outfile),
//                                      "-o",
//                                      fmt::format("{}.o", args.outfile)})
//                   .wait(),
//               0);

//     mk::lld::elf::ScopedLink{}({"ld",
//                                 "-dynamic-linker=/lib64/ld-linux-x86-64.so.2",
//                                 fmt::format("-o{}.out", args.outfile),
//                                 "-L/lib/x86_64-linux-gnu/",
//                                 "-Ltest/lib/",
//                                 fmt::format("{}.o", args.outfile),
//                                 "-lmylib",
//                                 "/lib/x86_64-linux-gnu/crt1.o",
//                                 "-lc",
//                                 "-rpath=."});

//     ASSERT_EQ(
//         Poco::Process::launch(fmt::format("./{}.out", args.outfile), {}).wait(),
//         82);
// }

// TEST(driver, executable)
// {
//     using namespace std::literals;
//     using namespace mk;

//     const std::string_view code = R"CODE(
//         extern bar(a,b)
//         def main()
//             1 + (2*3+4) + 4 * 5 + 6 * 3 + 42 - 3 - 33 + bar(1,2)
//     )CODE";

//     Driver driver;
//     Driver::Executable::Args args;
//     args.dynamic_linker = "/lib64/ld-linux-x86-64.so.2",
//     args.outfile = "output";
//     args.paths = {"/lib/x86_64-linux-gnu/", "test/lib"};
//     args.objects = {"/lib/x86_64-linux-gnu/crt1.o"};
//     args.libs = {"c", "mylib"};
//     args.rpath = {};

//     ASSERT_TRUE(driver(code, args));

//     ASSERT_EQ(
//         Poco::Process::launch(fmt::format("./{}.out", args.outfile), {}).wait(),
//         58);
// }