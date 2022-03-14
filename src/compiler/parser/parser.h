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
class Extern;
}  // namespace ast

class Parser
{
public:
    Parser(Lexer & lexer);

    const std::vector<std::unique_ptr<ast::Node>> & parse();

private:
    // prototype:= identifier(identifier ,identifier*)
    std::unique_ptr<ast::ProtoType> parse_proto_type();
    // extern := extern prototype
    std::unique_ptr<ast::Extern> parse_extern();
    // def := def prototype expr
    std::unique_ptr<ast::Node> parse_def();
    // expr := primary-expr | expr op expr
    std::unique_ptr<ast::Expr> parse_expr();
    // literal-expr := literal
    std::unique_ptr<ast::Expr> parse_literal_expr(double value);
    // identifier-expr := identifier | call-expr
    std::unique_ptr<ast::Expr> parse_identifier_expr(std::string && name);
    // primary-expr := (expr) | literal-expr | identifier-expr
    std::unique_ptr<ast::Expr> parse_primary_expr();
    // call-expr := identifier() | identifier(expr ,expr*)
    std::unique_ptr<ast::Expr> parse_call_expr(std::string && name);
    std::unique_ptr<ast::Expr>
    parse_bin_expr_rhs(std::int64_t previous,
                       std::unique_ptr<ast::Expr> && lhs);
    std::optional<ast::BinExpr::Op> parse_bin_op();
    std::optional<std::int64_t>
    get_precedence(const std::optional<ast::BinExpr::Op> & op);


    std::vector<std::unique_ptr<ast::Node>> root;
    Lexer & lexer;
};
}  // namespace mk

#endif