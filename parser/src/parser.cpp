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

// Implement precedence climbing
std::unique_ptr<Expression> Parser::parse_expression(int min_prec)
{
    std::unique_ptr<Expression> left;

    try {
        left = parse_factor();
        if (!has_tokens())
            return left;

        const Token* next_token = &peek();
        while (next_token && TokenTable::is_binary_operator(next_token->type()) && precedence(*next_token) >= min_prec) {
            BinaryOperator op = parse_binary_operator();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token) + 1);
            left = std::make_unique<BinaryExpression>(op, std::move(left), std::move(right));
            if (has_tokens()) {
                next_token = &peek();
            } else {
                next_token = nullptr;
            }
        }
        return left;
    } catch (const ParserError& e) {
        // Re-throw with expression context
        throw ParserError(std::format("In factor: {}", e.what()));
    } catch (const TokenError& e) {
        // Handle errors from token.literal<int>()
        throw ParserError(std::format("Invalid constant expression: {}", e.what()));
    }
}

std::unique_ptr<Expression> Parser::parse_factor()
{
    std::unique_ptr<Expression> res;

    try {
        const Token& next_token = peek();
        if (next_token.type() == TokenType::CONSTANT) {
            take_token();
            res = std::make_unique<ConstantExpression>(next_token.literal<int>());
            return res;
        } else if (TokenTable::is_unary_operator(next_token.type())) {
            UnaryOperator op = parse_unary_operator();
            std::unique_ptr<Expression> expr = parse_factor();
            res = std::make_unique<UnaryExpression>(op, std::move(expr));
            return res;
        } else if (next_token.type() == TokenType::OPEN_PAREN) {
            take_token();
            res = parse_expression();
            expect(TokenType::CLOSE_PAREN);
            return res;
        } else {
            throw ParserError("Malformed Factor");
        }

    } catch (const ParserError& e) {
        // Re-throw with expression context
        throw ParserError(std::format("In factor: {}", e.what()));
    } catch (const TokenError& e) {
        // Handle errors from token.literal<int>()
        throw ParserError(std::format("Invalid constant expression: {}", e.what()));
    }
}

UnaryOperator Parser::parse_unary_operator()
{
    static const std::unordered_map<TokenType, UnaryOperator> unary_op_map = {
        { TokenType::MINUS, UnaryOperator::NEGATE },
        { TokenType::COMPLEMENT, UnaryOperator::COMPLEMENT },
        { TokenType::EXCLAMATION_POINT, UnaryOperator::NOT }
    };

    const Token& next_token = peek();
    auto it = unary_op_map.find(next_token.type());
    if (it != unary_op_map.end()) {
        take_token();
        return it->second;
    }
    throw ParserError(std::format("In UnaryOperator: got {}", Token::type_to_string(next_token.type())));
}

BinaryOperator Parser::parse_binary_operator()
{
    static const std::unordered_map<TokenType, BinaryOperator> binary_op_map = {
        { TokenType::ASTERISK, BinaryOperator::MULTIPLY },
        { TokenType::FORWARD_SLASH, BinaryOperator::DIVIDE },
        { TokenType::PERCENT, BinaryOperator::REMAINDER },
        { TokenType::PLUS, BinaryOperator::ADD },
        { TokenType::MINUS, BinaryOperator::SUBTRACT },
        { TokenType::LOGICAL_AND, BinaryOperator::AND },
        { TokenType::LOGICAL_OR, BinaryOperator::OR },
        { TokenType::EQUAL, BinaryOperator::EQUAL },
        { TokenType::NOT_EQUAL, BinaryOperator::NOT_EQUAL },
        { TokenType::LESS_THAN, BinaryOperator::LESS_THAN },
        { TokenType::LESS_THAN_EQUAL, BinaryOperator::LESS_OR_EQUAL },
        { TokenType::GREATER_THAN, BinaryOperator::GREATER_THAN },
        { TokenType::GREATER_THAN_EQUAL, BinaryOperator::GREATER_OR_EQUAL }
    };

    const Token& next_token = peek();
    auto it = binary_op_map.find(next_token.type());
    if (it != binary_op_map.end()) {
        take_token();
        return it->second;
    }
    throw ParserError(std::format("In BinaryOperator: got {}", Token::type_to_string(next_token.type())));
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

int Parser::precedence(const Token& token)
{
    switch (token.type()) {
    case TokenType::ASTERISK:
    case TokenType::FORWARD_SLASH:
    case TokenType::PERCENT: {
        return 50;
    }
    case TokenType::PLUS:
    case TokenType::MINUS: {
        return 45;
    }
    case TokenType::LESS_THAN:
    case TokenType::LESS_THAN_EQUAL:
    case TokenType::GREATER_THAN:
    case TokenType::GREATER_THAN_EQUAL: {
        return 35;
    }
    case TokenType::EQUAL:
    case TokenType::NOT_EQUAL: {
        return 30;
    }
    case TokenType::LOGICAL_AND: {
        return 10;
    }
    case TokenType::LOGICAL_OR: {
        return 5;
    }
    default: {
        return 0;
    }
    }
}

void Parser::take_token()
{
    i++;
}

bool Parser::has_tokens()
{
    return i < m_tokens.size();
}
