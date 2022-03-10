#ifndef __LEXER_H__
#define __LEXER_H__

#include "token.h"

#include <string_view>


namespace mk
{
class Lexer
{
public:
    Lexer(const std::string_view & input);

    void next();
    const Token & current() const;

private:
    std::string_view input;
    Token token;
};
}  // namespace mk


#endif