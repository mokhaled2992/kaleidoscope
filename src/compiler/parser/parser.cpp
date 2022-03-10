#include "parser.h"

#include "ast.h"
#include "lexer.h"
#include "util/overload.h"


namespace mk
{
Parser::Parser(Lexer & lexer) : lexer(lexer) {}

std::unique_ptr<ast::Expr>
Parser::parse_bin_expr_rhs(std::unique_ptr<ast::Expr> && lhs)
{
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_call_expr(std::string && name)
{
    std::vector<std::unique_ptr<ast::Expr>> args;
    while (true)
    {
        if (std::holds_alternative<Empty>(lexer.current()))
        {
            break;
        }
        else if (std::holds_alternative<Right>(lexer.current()))
        {
            lexer.next();
            return std::make_unique<ast::CallExpr>(std::move(name),
                                                   std::move(args));
        }
        else if (auto arg = parse_expr())
        {
            args.emplace_back(std::move(arg));
            if (std::holds_alternative<Comma>(lexer.current()))
            {
                lexer.next();
            }
        }
        else
        {
            return nullptr;
        }
    }
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_identifier_expr(std::string && name)
{
    if (std::holds_alternative<Left>(lexer.current()))
    {
        lexer.next();
        return parse_call_expr(std::move(name));
    }
    else
    {
        auto lhs = std::make_unique<ast::Variable>(std::move(name));
        return parse_bin_expr_rhs(std::move(lhs));
    }
}

std::unique_ptr<ast::Expr> Parser::parse_literal_expr(double value)
{
    auto literal = std::make_unique<ast::Literal>(value);
    return parse_bin_expr_rhs(std::move(literal));
}

std::unique_ptr<ast::Expr> Parser::parse_expr()
{
    if (std::holds_alternative<Left>(lexer.current()))
    {
        lexer.next();
        auto expr = parse_expr();
        if (std::holds_alternative<Right>(lexer.current()))
        {
            return expr;
        }
        return nullptr;
    }
    else if (const auto p = std::get_if<Double>(&lexer.current()))
    {
        auto value = p->value;
        lexer.next();
        return parse_literal_expr(std::move(value));
    }
    else if (const auto p = std::get_if<Identifier>(&lexer.current()))
    {
        auto name = p->value;
        lexer.next();
        return parse_identifier_expr(std::move(name));
    }
    else if (std::holds_alternative<Invalid>(lexer.current()))
    {
        return nullptr;
    }
}

std::unique_ptr<ast::Node> Parser::parse_def()
{
    return nullptr;
}

std::unique_ptr<ast::Node> Parser::parse_extern()
{
    return nullptr;
}

const std::vector<std::unique_ptr<ast::Node>> & Parser::parse()
{
    lexer.next();

    while (true)
    {
        if (std::holds_alternative<Empty>(lexer.current()))
        {
            break;
        }
        else if (std::holds_alternative<Def>(lexer.current()))
        {
            root.emplace_back(parse_def());
        }
        else if (std::holds_alternative<Extern>(lexer.current()))
        {
            root.emplace_back(parse_extern());
        }
        else if (std::holds_alternative<Invalid>(lexer.current()))
        {
           return {};
        }
        else
        {
            root.emplace_back(parse_expr());
        }
    }

    return root;
}

}  // namespace mk