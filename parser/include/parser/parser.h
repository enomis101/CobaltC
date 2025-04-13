#pragma once
#include "common/data/token.h"
#include <memory>
#include "parser/parser_ast.h"

namespace parser{

class Parser{
public:
    Parser(const std::vector<Token>& tokens)
        : m_tokens{tokens}{}

    std::unique_ptr<Program> parse_program();

private:
    const std::vector<Token>& m_tokens;
    
    std::unique_ptr<Function> parse_function();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_expression();
    //std::unique_ptr<Identifier> parse_identifier();

    void expect(TokenType expected);
    int i = 0;
};
}