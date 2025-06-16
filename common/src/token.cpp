#include "common/data/token.h"
#include <sstream>

Token::Token(TokenType type, const std::string& lexeme, LiteralType literal, const SourceLocation& source_location)
    : m_type { type }
    , m_lexeme { lexeme }
    , m_literal(literal)
    , m_source_location { source_location }
{
}

std::string Token::type_to_string(TokenType type)
{
    switch (type) {
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::CONSTANT:
        return "CONSTANT";
    case TokenType::LONG_CONSTANT:
        return "LONG_CONSTANT";
    case TokenType::INT_KW:
        return "INT_KW";
    case TokenType::LONG_KW:
        return "LONG_KW";
    case TokenType::VOID_KW:
        return "VOID_KW";
    case TokenType::RETURN_KW:
        return "RETURN_KW";
    case TokenType::IF_KW:
        return "IF_KW";
    case TokenType::ELSE_KW:
        return "ELSE_KW";
    case TokenType::DO_KW:
        return "DO_KW";
    case TokenType::WHILE_KW:
        return "WHILE_KW";
    case TokenType::FOR_KW:
        return "FOR_KW";
    case TokenType::BREAK_KW:
        return "BREAK_KW";
    case TokenType::CONTINUE_KW:
        return "CONTINUE_KW";
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
    // UNARY_OPERATORS
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::DECREMENT:
        return "DECREMENT";
    case TokenType::COMPLEMENT:
        return "COMPLEMENT";
    case TokenType::EXCLAMATION_POINT:
        return "EXCLAMATION_POINT";
    // BINARY_OPERATORS
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::ASTERISK:
        return "ASTERISK";
    case TokenType::FORWARD_SLASH:
        return "FORWARD_SLASH";
    case TokenType::PERCENT:
        return "PERCENT";
    // LOGICAL_OPERATORS
    case TokenType::LOGICAL_AND:
        return "LOGICAL_AND";
    case TokenType::LOGICAL_OR:
        return "LOGICAL_OR";
    // COMPARISON_OPERATORS
    case TokenType::EQUAL:
        return "EQUAL";
    case TokenType::NOT_EQUAL:
        return "NOT_EQUAL";
    case TokenType::LESS_THAN:
        return "LESS_THAN";
    case TokenType::GREATER_THAN:
        return "GREATER_THAN";
    case TokenType::LESS_THAN_EQUAL:
        return "LESS_THAN_EQUAL";
    case TokenType::GREATER_THAN_EQUAL:
        return "GREATER_THAN_EQUAL";
    case TokenType::ASSIGNMENT:
        return "ASSIGNMENT";
    // CONDITIONAL OPERATORS
    case TokenType::QUESTION_MARK:
        return "QUESTION_MARK";
    case TokenType::COLON:
        return "COLON";
    // OTHERS
    case TokenType::COMMA:
        return "COMMA";
    default:
        return "UNKNOWN";
    }
}

std::string Token::to_string() const
{
    std::stringstream ss;
    ss << "Token{type=" << type_to_string(m_type)
       << ", lexeme='" << m_lexeme << "'"
       << ", line=" << m_source_location.line_number;

    // Handle the variant literal
    if (std::holds_alternative<int>(m_literal)) {
        ss << ", literal=" << std::get<int>(m_literal);
    } else if (std::holds_alternative<long>(m_literal)) {
        ss << ", literal=" << std::get<long>(m_literal);
    } else if (std::holds_alternative<unsigned int>(m_literal)) {
        ss << ", literal=" << std::get<unsigned int>(m_literal);
    } else if (std::holds_alternative<unsigned long>(m_literal)) {
        ss << ", literal=" << std::get<unsigned long>(m_literal);
    }

    ss << "}";
    return ss.str();
}
