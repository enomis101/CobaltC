#include "parser/parser.h"
#include <format>

using namespace parser;

std::unique_ptr<Program> parser::Parser::parse_program()
{
    std::unique_ptr<FunctionDefinition> fun = parse_function();
    if(has_tokens()){
        throw ParserError("Extra tokens left");
    }
    return std::make_unique<Program>(std::move(fun));
}

std::unique_ptr<FunctionDefinition> parser::Parser::parse_function()
{
    std::unique_ptr<FunctionDefinition> res;
   
    expect(TokenType::INT_KW);
    const Token& identifier_token = expect(TokenType::IDENTIFIER);
    std::unique_ptr<Identifier> identifier = std::make_unique<Identifier>(identifier_token.lexeme());
    expect(TokenType::OPEN_PAREN);
    expect(TokenType::VOID_KW);
    expect(TokenType::CLOSE_PAREN);
    expect(TokenType::OPEN_BRACE);
    std::unique_ptr<Statement> stm = parse_statement();
    expect(TokenType::CLOSE_BRACE);
    
    res = std::make_unique<Function>(std::move(identifier), std::move(stm));
    return res;
}

std::unique_ptr<Statement> parser::Parser::parse_statement()
{
    std::unique_ptr<Statement> res;
   
    expect(TokenType::RETURN_KW);
    std::unique_ptr<Expression> expr = parse_expression();
    expect(TokenType::SEMICOLON);
    res = std::make_unique<ReturnStatement>(std::move(expr));
    return res;
}

std::unique_ptr<Expression> parser::Parser::parse_expression()
{
    std::unique_ptr<Expression> res;
   
    const Token& curr = expect(TokenType::CONSTANT);
    res = std::make_unique<ConstantExpression>(curr.literal<int>());
    return res;
}

const Token& parser::Parser::expect(TokenType expected)
{
    const Token& actual = take_token();
    if(actual.type() != expected){
        throw ParserError(std::format("Unexpected token {}", actual.to_string()));
    }
    return actual;
}

const Token& parser::Parser::take_token()
{
    if(i >= m_tokens.size()){
        throw ParserError("Tried to consume token when None remaining!");
    }
    return m_tokens[i++];
}


bool parser::Parser::has_tokens()
{
    return i < m_tokens.size();
}
