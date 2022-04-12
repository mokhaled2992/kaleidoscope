#include "parser.h"

#include "ast.h"
#include "lexer.h"
#include "util/overload.h"

#include <optional>


namespace mk
{
Parser::Parser(Lexer & lexer) : lexer(lexer) {}


std::optional<ast::BinExpr::Op> Parser::parse_bin_op()
{
    std::optional<ast::BinExpr::Op> op;
    if (std::holds_alternative<Add>(lexer.current()))
    {
        op = ast::BinExpr::Op::Add;
    }
    else if (std::holds_alternative<Minus>(lexer.current()))
    {
        op = ast::BinExpr::Op::Minus;
    }
    else if (std::holds_alternative<Multiply>(lexer.current()))
    {
        op = ast::BinExpr::Op::Multiply;
    }
    else if (std::holds_alternative<LessThan>(lexer.current()))
    {
        op = ast::BinExpr::Op::LessThan;
    }

    return op;
}

std::optional<std::int64_t>
Parser::get_precedence(const std::optional<ast::BinExpr::Op> & op)
{
    if (!op)
        return {};

    auto it = ast::BinExpr::precedence.find(*op);
    if (it == ast::BinExpr::precedence.cend())
        return {};
    return it->second;
}

std::unique_ptr<ast::Expr>
Parser::parse_bin_expr_rhs(std::int64_t previous,
                           std::unique_ptr<ast::Expr> && lhs)
{
    while (true)
    {
        auto current = parse_bin_op();

        if (get_precedence(current) < previous)
            return std::move(lhs);

        lexer.next();

        auto rhs = parse_primary_expr();

        auto next = parse_bin_op();

        if (get_precedence(next) > get_precedence(current))
        {
            rhs = parse_bin_expr_rhs(*get_precedence(current) + 1,
                                     std::move(rhs));
        }
        lhs = std::make_unique<ast::BinExpr>(std::move(*current),
                                             std::move(lhs),
                                             std::move(rhs));
    }
}

std::unique_ptr<ast::Expr> Parser::parse_call_expr(std::string && name)
{
    std::vector<std::unique_ptr<ast::Expr>> args;
    while (true)
    {
        if (std::holds_alternative<Right>(lexer.current()))
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
        return std::make_unique<ast::Variable>(std::move(name));
    }
}

std::unique_ptr<ast::Expr> Parser::parse_literal_expr(double value)
{
    return std::make_unique<ast::Literal>(value);
}

std::unique_ptr<ast::Expr> Parser::parse_primary_expr()
{
    if (std::holds_alternative<Left>(lexer.current()))
    {
        lexer.next();
        auto expr = parse_expr();
        if (std::holds_alternative<Right>(lexer.current()))
        {
            lexer.next();
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
    else if (std::holds_alternative<If>(lexer.current()))
    {
        lexer.next();
        if (std::holds_alternative<Left>(lexer.current()))
        {
            lexer.next();
            auto condition = parse_expr();
            if (std::holds_alternative<Right>(lexer.current()))
            {
                lexer.next();
                if (std::holds_alternative<Then>(lexer.current()))
                {
                    lexer.next();
                    auto first = parse_expr();
                    if (std::holds_alternative<Else>(lexer.current()))
                    {
                        lexer.next();
                        auto second = parse_expr();

                        return std::make_unique<ast::ConditionalExpr>(
                            std::move(condition),
                            std::move(first),
                            std::move(second));
                    }
                }
            }
        }
        return nullptr;
    }
    else if (std::holds_alternative<For>(lexer.current()))
    {
        lexer.next();
        if (const auto p = std::get_if<Identifier>(&lexer.current()))
        {
            auto name = p->value;
            lexer.next();
            if (std::holds_alternative<Assignment>(lexer.current()))
            {
                lexer.next();
                auto init = parse_expr();
                if (std::holds_alternative<Comma>(lexer.current()))
                {
                    lexer.next();
                    auto condition = parse_expr();

                    std::unique_ptr<ast::Expr> step;
                    if (std::holds_alternative<Comma>(lexer.current()))
                    {
                        lexer.next();
                        step = parse_expr();
                    }

                    if (std::holds_alternative<In>(lexer.current()))
                    {
                        lexer.next();
                        auto body = parse_expr();
                        return std::make_unique<ast::ForExpr>(std::move(name),
                                                              std::move(init),
                                                              std::move(
                                                                  condition),
                                                              std::move(step),
                                                              std::move(body));
                    }
                }
            }
        }
        return nullptr;
    }
    else if (std::holds_alternative<Invalid>(lexer.current()))
    {
        return nullptr;
    }
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_expr()
{
    if (auto lhs = parse_primary_expr())
    {
        return parse_bin_expr_rhs(0, std::move(lhs));
    }

    return nullptr;
}

std::unique_ptr<ast::Node> Parser::parse_def()
{
    if (auto signature = parse_proto_type())
    {
        return std::make_unique<ast::Function>(std::move(signature),
                                               parse_expr());
    }

    return nullptr;
}

std::unique_ptr<ast::ProtoType> Parser::parse_proto_type()
{
    std::string name;
    std::vector<std::string> params;
    if (const auto p = std::get_if<Identifier>(&lexer.current()))
    {
        name = std::move(p->value);
        lexer.next();
        if (std::holds_alternative<Left>(lexer.current()))
        {
            lexer.next();
            while (true)
            {
                if (std::holds_alternative<Right>(lexer.current()))
                {
                    lexer.next();
                    return std::make_unique<ast::ProtoType>(std::move(name),
                                                            std::move(params));
                }
                else if (const auto p =
                             std::get_if<Identifier>(&lexer.current()))
                {
                    params.push_back(std::move(p->value));
                    lexer.next();
                    if (std::holds_alternative<Comma>(lexer.current()))
                        lexer.next();
                }
            }
        }
    }
    return nullptr;
}

std::unique_ptr<ast::Extern> Parser::parse_extern()
{
    if (auto p = parse_proto_type())
        return std::make_unique<ast::Extern>(std::move(p));
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
        else if (auto p = std::get_if<Invalid>(&lexer.current()))
        {
            root.clear();
            root.emplace_back(
                std::make_unique<ast::Error>(std::move(p->value)));
            break;
        }
        else if (std::holds_alternative<Def>(lexer.current()))
        {
            lexer.next();
            root.emplace_back(parse_def());
        }
        else if (std::holds_alternative<Extern>(lexer.current()))
        {
            lexer.next();
            root.emplace_back(parse_extern());
        }
        else
        {
            root.emplace_back(parse_expr());
        }
    }

    return root;
}

}  // namespace mk