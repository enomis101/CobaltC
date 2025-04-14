#include "common/data/token.h"
#include <format>
#include <sstream>

Token::Token(const std::string& lexeme, int line)
    : m_lexeme { lexeme }
    , m_line { line }
{
    TokenTable& tb = TokenTable::instance();
    std::optional<TokenType> result = tb.match(lexeme);
    if (!result.has_value()) {
        throw TokenError("Failed to match valid token in Token constructor");
    }

    m_type = result.value();

    if (m_type == TokenType::CONSTANT) {
        // With error handling
        try {
            int num = std::stoi(m_lexeme); // This will throw an exception
            m_literal = num;
        } catch (const std::exception& e) {
            throw TokenError(std::format("Error while parsing constant in Token constructor: {}", e.what()));
        }
    }
}

std::string Token::type_to_string(TokenType type)
{
    switch (type) {
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::CONSTANT:
        return "CONSTANT";
    case TokenType::INT_KW:
        return "INT_KW";
    case TokenType::VOID_KW:
        return "VOID_KW";
    case TokenType::RETURN_KW:
        return "RETURN_KW";
    case TokenType::OPEN_PAREN:
        return "OPEN_PAREN";
    case TokenType::CLOSE_PAREN:
        return "CLOSE_PAREN";
    case TokenType::OPEN_BRACE:
        return "OPEN_BRACE";
    case TokenType::CLOSE_BRACE:
        return "CLOSE_BRACE";
    case TokenType::SEMICOLON:
        return "SEMICOLON";
    default:
        return "UNKNOWN";
    }
}

std::string Token::to_string() const
{
    std::stringstream ss;
    ss << "Token{type=" << type_to_string(m_type)
       << ", lexeme='" << m_lexeme << "'"
       << ", line=" << m_line;

    // Handle the variant literal
    if (std::holds_alternative<int>(m_literal)) {
        ss << ", literal=" << std::get<int>(m_literal);
    }

    ss << "}";
    return ss.str();
}
