#ifndef __PARSER_H__
#define __PARSER_H__

#include <memory>
#include <vector>

namespace mk
{

class Lexer;

namespace ast
{
class Node;
class Expr;
}

class Parser
{
public:
    Parser(Lexer & lexer);

    const std::vector<std::unique_ptr<ast::Node>> & parse();

private:
    std::unique_ptr<ast::Node> parse_extern();
    std::unique_ptr<ast::Node> parse_def();

    std::unique_ptr<ast::Expr> parse_expr();
    std::unique_ptr<ast::Expr> parse_identifier_expr(std::string && name);
    std::unique_ptr<ast::Expr> parse_call_expr(std::string && name);
    std::unique_ptr<ast::Expr> parse_literal_expr(double value);
    std::unique_ptr<ast::Expr>
    parse_bin_expr_rhs(std::unique_ptr<ast::Expr> && lhs);


    std::vector<std::unique_ptr<ast::Node>> root;
    Lexer & lexer;
};
}  // namespace mk

#endif