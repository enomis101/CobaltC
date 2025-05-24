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

    std::unique_ptr<Declaration> parse_declaration();
    std::unique_ptr<Block> parse_block();
    std::unique_ptr<BlockItem> parse_block_item();
    std::unique_ptr<ForInit> parse_for_init();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_conditional_middle();
    std::unique_ptr<Expression> parse_expression(int min_prec = 0);
    std::unique_ptr<Expression> parse_factor();
    std::vector<Identifier> parse_parameter_list();

    std::pair<std::unique_ptr<Type>, StorageClass> parse_type_and_storage_class();

    UnaryOperator parse_unary_operator();
    BinaryOperator parse_binary_operator();
    StorageClass to_storage_class(TokenType tt);

    // Utility methods to check token types
    bool is_binary_operator(TokenType type);
    bool is_unary_operator(TokenType type);
    bool is_specificer(TokenType type);

    // std::unique_ptr<Identifier> parse_identifier();

    const Token& expect(TokenType expected);
    const Token& peek(int lh = 1);
    int precedence(const Token& token);
    void take_token();
    bool has_tokens();
    size_t i = 0;
};
}
