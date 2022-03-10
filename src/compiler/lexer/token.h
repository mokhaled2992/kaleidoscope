#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <string>
#include <string_view>
#include <variant>

namespace mk
{
struct TokenBase
{
};

struct Def : TokenBase
{
    bool operator==(const Def & other) const { return true; }
    static constexpr const char * const value = "def";
};

struct Extern : TokenBase
{
    bool operator==(const Extern & other) const { return true; }
    static constexpr const char * const value = "extern";
};

struct Identifier : TokenBase
{
    Identifier(std::string && value) : value(std::move(value)) {}
    bool operator==(const Identifier & other) const
    {
        return value == other.value;
    }
    std::string value;
};

struct Literal : TokenBase
{
};

struct Double : Literal
{
    Double(double value) : value(value) {}
    bool operator==(const Double & other) const { return value == other.value; }
    double value;
};

struct Operator : TokenBase
{
};

struct Add : Operator
{
    bool operator==(const Add & other) const { return true; }
    static constexpr unsigned char value = '+';
};

struct Minus : Operator
{
    bool operator==(const Minus & other) const { return true; }
    static constexpr unsigned char value = '-';
};

struct Multiply : Operator
{
    bool operator==(const Multiply & other) const { return true; }
    static constexpr unsigned char value = '*';
};

struct LessThan : Operator
{
    bool operator==(const LessThan & other) const { return true; }
    static constexpr unsigned char value = '<';
};

struct Parenthesis : TokenBase
{
};

struct Left : Parenthesis
{
    bool operator==(const Left & other) const { return true; }
    static constexpr unsigned char value = '(';
};

struct Right : Parenthesis
{
    bool operator==(const Right & other) const { return true; }
    static constexpr unsigned char value = ')';
};

struct Invalid : TokenBase
{
    Invalid(std::string && value) : value(std::move(value)) {}
    bool operator==(const Invalid & other) const
    {
        return value == other.value;
    }
    std::string value;
};

struct Comment : TokenBase
{
    static constexpr unsigned char value = '#';
};

struct CarriageReturn : TokenBase
{
    static constexpr unsigned char value = '\r';
};

struct FloatingPoint : TokenBase
{
    static constexpr unsigned char value = '.';
};

struct NewLine : TokenBase
{
    static constexpr unsigned char value = '\n';
};

struct Empty : TokenBase
{
    bool operator==(const Empty &) const { return true; }
};

struct SemiColon : TokenBase
{
    bool operator==(const SemiColon &) const { return true; }
    static constexpr unsigned char value = ';';
};

struct Comma : TokenBase
{
    bool operator==(const Comma &) const { return true; }
    static constexpr unsigned char value = ',';
};

using Token = std::variant<Empty,
                           Def,
                           Extern,
                           Identifier,
                           Double,
                           Add,
                           Minus,
                           Multiply,
                           LessThan,
                           Left,
                           Right,
                           SemiColon,
                           Comma,
                           Invalid>;
}  // namespace mk

#endif