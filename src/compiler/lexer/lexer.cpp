#include "lexer.h"

#include <cctype>
#include <cstdlib>

#include <iostream>
namespace mk
{
Lexer::Lexer(const std::string_view & input) : input(input), token() {}

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
        else if (c == '#')
        {
            while (!input.empty() && c != '\n' && c != '\r')
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
            else if (value == For::value)
            {
                token = For();
            }
            else if (value == In::value)
            {
                token = In();
            }
            else if (value == "operator")
            {
                while (!input.empty() && std::isspace(c = input.front()))
                    input.remove_prefix(1);
                std::string opcode;
                while (!input.empty() && !std::isspace(c = input.front()))
                {
                    opcode += c;
                    input.remove_prefix(1);
                }
                token = Operator(std::move(opcode));
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

            if (c == '.')
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
                token = std::stod(value);
            }
        }
        else
        {
            token = c;
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