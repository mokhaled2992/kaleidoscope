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

struct Invalid : TokenBase
{
    Invalid(std::string && value) : value(std::move(value)) {}
    bool operator==(const Invalid & other) const
    {
        return value == other.value;
    }
    std::string value;
};

struct Empty : TokenBase
{
    bool operator==(const Empty &) const { return true; }
};

struct If : TokenBase
{
    bool operator==(const If &) const { return true; }
    constexpr static const char * const value = "if";
};

struct Then : TokenBase
{
    bool operator==(const Then &) const { return true; }
    constexpr static const char * const value = "then";
};

struct Else : TokenBase
{
    bool operator==(const Else &) const { return true; }
    constexpr static const char * const value = "else";
};

struct For : TokenBase
{
    bool operator==(const For &) const { return true; }
    constexpr static const char * const value = "for";
};

struct In : TokenBase
{
    bool operator==(const In &) const { return true; }
    constexpr static const char * const value = "in";
};

struct Let : TokenBase
{
    bool operator==(const Let &) const { return true; }
    constexpr static const char * const value = "let";
};

struct Operator : TokenBase
{
    bool operator==(const Operator & other) const
    {
        return value == other.value;
    }
    Operator(std::string && value) : value(std::move(value)) {}
    std::string value;
};

using TokenType = std::variant<Empty,
                               Def,
                               Extern,
                               Identifier,
                               If,
                               Then,
                               Else,
                               For,
                               In,
                               Operator,
                               Let,
                               double,
                               unsigned char,
                               Invalid>;
struct Token : TokenType
{
    using TokenType::variant;
    using TokenType::operator=;

    bool is(unsigned char c) const
    {
        const auto p =
            std::get_if<unsigned char>(static_cast<const TokenType *>(this));
        return p && *p == c;
    }

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T>(static_cast<const TokenType &>(*this));
    }
};
}  // namespace mk

#endif