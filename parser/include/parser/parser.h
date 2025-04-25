#pragma once
#include "common/data/token.h"
#include "parser/parser_ast.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace parser {

class ParserError : public std::runtime_error {
public:
    explicit ParserError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class Parser {
public:
    Parser(const std::vector<Token>& tokens)
        : m_tokens { tokens }
    {
    }

    std::shared_ptr<Program> parse_program();

private:
    const std::vector<Token>& m_tokens;

    std::unique_ptr<Function> parse_function();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_expression(int min_prec = 0);
    std::unique_ptr<Expression> parse_factor();
    std::unique_ptr<UnaryOperator> parse_unary_operator();
    std::unique_ptr<BinaryOperator> parse_binary_operator();

    // std::unique_ptr<Identifier> parse_identifier();

    const Token& expect(TokenType expected);
    const Token& peek();
    int precedence(const Token& token);
    void take_token();
    bool has_tokens();
    size_t i = 0;
};
}
