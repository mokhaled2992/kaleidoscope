#include <gtest/gtest.h>

#include "compiler/lexer/lexer.h"
#include "util/overload.h"

#include <iostream>
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
