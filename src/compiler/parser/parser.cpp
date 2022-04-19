#include "parser.h"

#include "ast.h"
#include "lexer.h"
#include "util/overload.h"

#include <optional>


namespace mk
{

Parser::Parser(Lexer & lexer)
    : lexer(lexer)
    , precedence({
          {"=", 2},
          {"<", 10},
          {"+", 20},
          {"-", 20},
          {"*", 40},
      })
{}

void Parser::Precedence::push(const std::string & op, std::int64_t value)
{
    std::lock_guard lock(mutex);
    map.emplace(op, value);
}

std::optional<std::int64_t>
Parser::Precedence::get(const std::optional<std::string> & op) const
{
    if (!op)
        return std::nullopt;

    std::shared_lock lock(mutex);
    if (const auto it = map.find(*op); it != map.cend())
        return it->second;
    return std::nullopt;
}

std::optional<std::string> Parser::parse_bin_op()
{
    std::optional<std::string> op;
    if (const auto p = std::get_if<unsigned char>(&lexer.current()))
    {
        op = std::string(1, *p);
    }
    else if (const auto p = std::get_if<Identifier>(&lexer.current()))
    {
        op = p->value;
    }

    return op;
}

std::unique_ptr<ast::Expr>
Parser::parse_bin_expr_rhs(std::int64_t previous,
                           std::unique_ptr<ast::Expr> && lhs)
{
    while (true)
    {
        auto current = parse_bin_op();

        if (precedence.get(current) < previous)
            return std::move(lhs);

        lexer.next();

        auto rhs = parse_unary_expr();

        auto next = parse_bin_op();

        if (precedence.get(next) > precedence.get(current))
        {
            rhs = parse_bin_expr_rhs(*precedence.get(current) + 1,
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
        if (lexer.current().is(')'))
        {
            lexer.next();
            return std::make_unique<ast::CallExpr>(std::move(name),
                                                   std::move(args));
        }
        else if (auto arg = parse_expr())
        {
            args.emplace_back(std::move(arg));
            if (lexer.current().is(','))
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
    if (lexer.current().is('('))
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
    if (lexer.current().is('('))
    {
        lexer.next();
        auto expr = parse_expr();
        if (lexer.current().is(')'))
        {
            lexer.next();
            return expr;
        }
        return nullptr;
    }
    else if (const auto p = std::get_if<double>(&lexer.current()))
    {
        auto value = *p;
        lexer.next();
        return parse_literal_expr(std::move(value));
    }
    else if (const auto p = std::get_if<Identifier>(&lexer.current()))
    {
        auto name = p->value;
        lexer.next();
        return parse_identifier_expr(std::move(name));
    }
    else if (lexer.current().is<If>())
    {
        lexer.next();
        if (lexer.current().is('('))
        {
            lexer.next();
            auto condition = parse_expr();
            if (lexer.current().is(')'))
            {
                lexer.next();
                if (lexer.current().is<Then>())
                {
                    lexer.next();
                    auto first = parse_expr();
                    if (lexer.current().is<Else>())
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
    else if (lexer.current().is<For>())
    {
        lexer.next();
        if (const auto p = std::get_if<Identifier>(&lexer.current()))
        {
            auto name = p->value;
            lexer.next();
            if (lexer.current().is('='))
            {
                lexer.next();
                auto init = parse_expr();
                if (lexer.current().is(','))
                {
                    lexer.next();
                    auto condition = parse_expr();

                    std::unique_ptr<ast::Expr> step;
                    if (lexer.current().is(','))
                    {
                        lexer.next();
                        step = parse_expr();
                    }

                    if (lexer.current().is<In>())
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
    else if (lexer.current().is<Let>())
    {
        lexer.next();
        std::vector<std::pair<std::string, std::unique_ptr<ast::Expr>>> vars;
        while (const auto p = std::get_if<Identifier>(&lexer.current()))
        {
            auto name = p->value;
            lexer.next();
            if (lexer.current().is('='))
            {
                lexer.next();
                auto value = parse_expr();
                vars.emplace_back(std::move(name), std::move(value));
            }
        }

        if (lexer.current().is<In>())
        {
            lexer.next();
            auto body = parse_expr();
            return std::make_unique<ast::LetExpr>(std::move(vars),
                                                  std::move(body));
        }
    }
    else if (lexer.current().is<Invalid>())
    {
        return nullptr;
    }
    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_unary_expr()
{
    if (auto expr = parse_primary_expr())
        return expr;

    if (const auto p = std::get_if<unsigned char>(&lexer.current()))
    {
        if (std::ispunct(*p))
        {
            std::string op(1, *p);
            lexer.next();
            auto expr = parse_unary_expr();
            return std::make_unique<ast::UnaryExpr>(std::move(op),
                                                    std::move(expr));
        }
    }

    return nullptr;
}

std::unique_ptr<ast::Expr> Parser::parse_expr()
{
    if (auto lhs = parse_unary_expr())
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
    }
    else if (const auto p = std::get_if<Operator>(&lexer.current()))
    {
        name = std::move(p->value);
        lexer.next();
        if (const auto p = std::get_if<double>(&lexer.current()))
        {
            precedence.push(name, std::move(*p));
            lexer.next();
        }
    }

    if (lexer.current().is('('))
    {
        lexer.next();
        while (true)
        {
            if (lexer.current().is(')'))
            {
                lexer.next();
                return std::make_unique<ast::ProtoType>(std::move(name),
                                                        std::move(params));
            }
            else if (const auto p = std::get_if<Identifier>(&lexer.current()))
            {
                params.push_back(std::move(p->value));
                lexer.next();
                if (lexer.current().is(','))
                    lexer.next();
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
        if (lexer.current().is<Empty>())
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
        else if (lexer.current().is<Def>())
        {
            lexer.next();
            root.emplace_back(parse_def());
        }
        else if (lexer.current().is<Extern>())
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