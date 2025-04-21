#include "parser/parser.h"
#include <format>
#include <sstream>
#include <vector>

using namespace parser;

std::shared_ptr<Program> Parser::parse_program()
{
    try {
        std::unique_ptr<Function> fun = parse_function();
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

std::unique_ptr<Function> Parser::parse_function()
{
    std::unique_ptr<Function> res;

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

std::unique_ptr<Statement> Parser::parse_statement()
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

std::unique_ptr<Expression> Parser::parse_expression()
{
    std::unique_ptr<Expression> res;

    try {
        const Token& next_token = peek();
        if (next_token.type() == TokenType::CONSTANT) {
            take_token();
            res = std::make_unique<ConstantExpression>(next_token.literal<int>());
            return res;
        } else if (next_token.type() == TokenType::MINUS || next_token.type() == TokenType::COMPLEMENT) {
            std::unique_ptr<UnaryOperator> op = parse_unary_operator();
            std::unique_ptr<Expression> expr = parse_expression();
            res = std::make_unique<UnaryExpression>(std::move(op), std::move(expr));
            return res;
        } else if (next_token.type() == TokenType::OPEN_PAREN) {
            take_token();
            res = parse_expression();
            expect(TokenType::CLOSE_PAREN);
            return res;
        } else {
            throw ParserError("Malformed expression");
        }

    } catch (const ParserError& e) {
        // Re-throw with expression context
        throw ParserError(std::format("In expression: {}", e.what()));
    } catch (const TokenError& e) {
        // Handle errors from token.literal<int>()
        throw ParserError(std::format("Invalid constant expression: {}", e.what()));
    }
}

std::unique_ptr<UnaryOperator> Parser::parse_unary_operator()
{
    std::unique_ptr<UnaryOperator> res;
    const Token& next_token = peek();
    if (next_token.type() == TokenType::MINUS) {
        take_token();
        res = std::make_unique<NegateOperator>();
        return res;
    } else if (next_token.type() == TokenType::COMPLEMENT) {
        take_token();
        res = std::make_unique<ComplementOperator>();
        return res;
    } else {
        throw ParserError(std::format("In UnaryOperator: got {}", Token::type_to_string(next_token.type())));
    }
}

const Token& Parser::expect(TokenType expected)
{
    if (!has_tokens()) {
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

const Token& Parser::peek()
{
    if (!has_tokens()) {
        throw ParserError("Unexpected end of file. Trying to peek.");
    }
    return m_tokens[i];
}

void Parser::take_token()
{
    i++;
}

bool Parser::has_tokens()
{
    return i < m_tokens.size();
}
