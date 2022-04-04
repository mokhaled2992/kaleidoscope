#include "lexer.h"

#include <cctype>
#include <cstdlib>

#include <iostream>
namespace mk
{
Lexer::Lexer(const std::string_view & input) : input(input), token(Empty{}) {}

void Lexer::next()
{
    while (!input.empty())
    {
        unsigned char c = input.front();
        input.remove_prefix(1);

        if (std::isspace(c))
        {
            continue;
        }
        else if (c == Comment::value)
        {
            while (!input.empty() && c != CarriageReturn::value
                   && c != NewLine::value)
            {
                c = input.front();
                input.remove_prefix(1);
            }
            continue;
        }
        else if (std::isalpha(c))
        {
            std::string value;
            value += c;

            while (!input.empty() && std::isalnum(c = input.front()))
            {
                value += c;
                input.remove_prefix(1);
            }

            if (value == Def::value)
            {
                token = Def();
            }
            else if (value == Extern::value)
            {
                token = Extern();
            }
            else if (value == If::value)
            {
                token = If();
            }
            else if (value == Then::value)
            {
                token = Then();
            }
            else if (value == Else::value)
            {
                token = Else();
            }
            else
            {
                token = Identifier(std::move(value));
            }
        }
        else if (std::isdigit(c))
        {
            std::string value;
            value += c;
            bool floating_point = false;

        number:
            while (!input.empty() && std::isdigit(c = input.front()))
            {
                value += c;
                input.remove_prefix(1);
            }

            if (c == FloatingPoint::value)
            {
                if (floating_point)
                {
                    token = Invalid("Invalid floating point number");
                }
                else
                {
                    floating_point = true;
                    value += c;
                    input.remove_prefix(1);
                    goto number;
                }
            }
            else
            {
                token = Double(std::stod(value));
            }
        }
        else if (c == Left::value)
        {
            token = Left();
        }
        else if (c == Right::value)
        {
            token = Right();
        }
        else if (c == Add::value)
        {
            token = Add();
        }
        else if (c == Minus::value)
        {
            token = Minus();
        }
        else if (c == Multiply::value)
        {
            token = Multiply();
        }
        else if (c == LessThan::value)
        {
            token = LessThan();
        }
        else if (c == SemiColon::value)
        {
            token = SemiColon();
        }
        else if (c == Comma::value)
        {
            token = Comma();
        }
        else
        {
            token = Invalid("Unknown Character");
        }
        return;
    }
    token = Empty{};
}

const Token & Lexer::current() const
{
    return token;
}


}  // namespace mk