#ifndef __PARSER_H__
#define __PARSER_H__

#include "ast.h"

#include <memory>
#include <optional>
#include <vector>

namespace mk
{

class Lexer;

namespace ast
{
class Node;
class Expr;
}  // namespace ast

class Parser
{
public:
    Parser(Lexer & lexer);

    const std::vector<std::unique_ptr<ast::Node>> & parse();

private:
    std::unique_ptr<ast::Node> parse_extern();
    std::unique_ptr<ast::Node> parse_def();
    std::optional<ast::BinExpr::Op> parse_bin_op();
    std::unique_ptr<ast::Expr> parse_expr();
    std::unique_ptr<ast::Expr> parse_primary_expr();
    std::unique_ptr<ast::Expr> parse_identifier_expr(std::string && name);
    std::unique_ptr<ast::Expr> parse_call_expr(std::string && name);
    std::unique_ptr<ast::Expr> parse_literal_expr(double value);
    std::unique_ptr<ast::Expr>
    parse_bin_expr_rhs(std::int64_t previous,
                       std::unique_ptr<ast::Expr> && lhs);
    std::optional<std::int64_t>
    get_precedence(const std::optional<ast::BinExpr::Op> & op);


    std::vector<std::unique_ptr<ast::Node>> root;
    Lexer & lexer;
};
}  // namespace mk

#endif