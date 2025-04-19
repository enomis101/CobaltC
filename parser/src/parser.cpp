#include "parser/parser.h"
#include <format>
#include <sstream>
#include <vector>

using namespace parser;

std::shared_ptr<Program> parser::Parser::parse_program()
{
    try {
        std::unique_ptr<FunctionDefinition> fun = parse_function();
        if (has_tokens()) {
            const Token& extra_token = m_tokens[i];
            throw ParserError(std::format(
                "Unexpected token after program end at line {}: '{}'",
                extra_token.line(), extra_token.lexeme()));
        }
        return std::make_shared<Program>(std::move(fun));
    } catch (const ParserError& e) {
        // Re-throw with additional context
        throw ParserError(std::format("Failed to parse program: {}", e.what()));
    }
}

std::unique_ptr<FunctionDefinition> parser::Parser::parse_function()
{
    std::unique_ptr<FunctionDefinition> res;

    try {
        // Track the function we're trying to parse
        std::string function_name = "unknown";

        expect(TokenType::INT_KW);
        const Token& identifier_token = expect(TokenType::IDENTIFIER);
        function_name = identifier_token.lexeme();

        std::unique_ptr<Identifier> identifier = std::make_unique<Identifier>(function_name);

        expect(TokenType::OPEN_PAREN);
        expect(TokenType::VOID_KW);
        expect(TokenType::CLOSE_PAREN);
        expect(TokenType::OPEN_BRACE);
        std::unique_ptr<Statement> stm = parse_statement();
        expect(TokenType::CLOSE_BRACE);

        res = std::make_unique<Function>(std::move(identifier), std::move(stm));
        return res;
    } catch (const ParserError& e) {
        // Re-throw with function context
        throw ParserError(std::format("In function definition: {}", e.what()));
    }
}

std::unique_ptr<Statement> parser::Parser::parse_statement()
{
    std::unique_ptr<Statement> res;

    try {
        expect(TokenType::RETURN_KW);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::SEMICOLON);
        res = std::make_unique<ReturnStatement>(std::move(expr));
        return res;
    } catch (const ParserError& e) {
        // Re-throw with statement context
        throw ParserError(std::format("In statement: {}", e.what()));
    }
}

std::unique_ptr<Expression> parser::Parser::parse_expression()
{
    std::unique_ptr<Expression> res;

    try {
        const Token& curr = expect(TokenType::CONSTANT);
        res = std::make_unique<ConstantExpression>(curr.literal<int>());
        return res;
    } catch (const ParserError& e) {
        // Re-throw with expression context
        throw ParserError(std::format("In expression: {}", e.what()));
    } catch (const TokenError& e) {
        // Handle errors from token.literal<int>()
        throw ParserError(std::format("Invalid constant expression: {}", e.what()));
    }
}

const Token& parser::Parser::expect(TokenType expected)
{
    if (i >= m_tokens.size()) {
        throw ParserError(std::format(
            "Unexpected end of file. Expected: {}",
            Token::type_to_string(expected)));
    }

    const Token& actual = m_tokens[i++];
    if (actual.type() != expected) {
        throw ParserError(std::format(
            "Syntax error at line {}: Expected '{}' but found '{}'",
            actual.line(),
            Token::type_to_string(expected),
            actual.lexeme()));
    }
    return actual;
}

bool parser::Parser::has_tokens()
{
    return i < m_tokens.size();
}
