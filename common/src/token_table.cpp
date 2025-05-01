#include "common/data/token_table.h"

// Meyer's Singleton implementation
TokenTable& TokenTable::instance()
{
    static TokenTable s_instance;
    return s_instance;
}

size_t TokenTable::search(std::string_view input) const
{
    if (input.size() <= 0)
        return 0;

    char first_char = input[0];

    // Check for double character tokens if input has at least 2 characters
    if (input.size() >= 2) {
        std::string_view double_char = input.substr(0, 2);
        if (m_double_char_tokens.contains(double_char)) {
            return 2;
        }
    }

    if (m_single_char_tokens.contains(first_char)) {
        return 1;
    }

    for (const auto& p : m_patterns) {
        const boost::regex& pattern = p.first;
        boost::match_results<std::string_view::const_iterator> matches;
        if (boost::regex_search(input.begin(), input.end(), matches, pattern)) {
            // Make sure the match is at the beginning
            if (matches.position() == 0) {
                // Return the length of the matched text
                return matches.length();
            }
        }
    }

    return 0;
}

std::optional<TokenType> TokenTable::match(std::string_view lexeme) const
{
    std::optional<TokenType> res;
    if (lexeme.size() <= 0)
        return res;

    if (lexeme.size() == 1) {
        char single_char = lexeme[0];
        if (m_single_char_tokens.contains(single_char)) {
            res = m_single_char_tokens.at(single_char);
            return res;
        }
    }

    if (lexeme.size() == 2) {
        if (m_double_char_tokens.contains(lexeme)) {
            res = m_double_char_tokens.at(lexeme);
            return res;
        }
    }

    for (const auto& p : m_patterns) {
        const boost::regex& pattern = p.first;
        TokenType type = p.second;
        boost::match_results<std::string_view::const_iterator> matches;
        if (boost::regex_match(lexeme.begin(), lexeme.end(), matches, pattern)) {
            res = type;
            if (type == TokenType::IDENTIFIER && m_keywords.contains(lexeme)) {
                res = m_keywords.at(lexeme);
            }
            return res;
        }
    }

    return res;
}

bool TokenTable::is_binary_operator(TokenType type)
{
    switch (type) {
    case TokenType::PLUS:
    case TokenType::MINUS:
    case TokenType::ASTERISK:
    case TokenType::FORWARD_SLASH:
    case TokenType::PERCENT:
    case TokenType::LOGICAL_AND:
    case TokenType::LOGICAL_OR:
    case TokenType::EQUAL:
    case TokenType::NOT_EQUAL:
    case TokenType::LESS_THAN:
    case TokenType::GREATER_THAN:
    case TokenType::LESS_THAN_EQUAL:
    case TokenType::GREATER_THAN_EQUAL:
        // Add other binary operators as needed
        return true;
    default:
        return false;
    }
}

bool TokenTable::is_unary_operator(TokenType type)
{
    switch (type) {
    case TokenType::MINUS:
    case TokenType::COMPLEMENT:
    case TokenType::DECREMENT:
    case TokenType::EXCLAMATION_POINT:
        // Add other unary operators as needed
        return true;
    default:
        return false;
    }
}

TokenTable::TokenTable()
{
    // Initialize keywords
    m_keywords = {
        { "int", TokenType::INT_KW },
        { "void", TokenType::VOID_KW },
        { "return", TokenType::RETURN_KW }
    };

    // Initialize single-character tokens for optimization
    m_single_char_tokens = {
        { '(', TokenType::OPEN_PAREN },
        { ')', TokenType::CLOSE_PAREN },
        { '{', TokenType::OPEN_BRACE },
        { '}', TokenType::CLOSE_BRACE },
        { ';', TokenType::SEMICOLON },
        { '-', TokenType::MINUS },
        { '~', TokenType::COMPLEMENT },
        { '+', TokenType::PLUS },
        { '*', TokenType::ASTERISK },
        { '/', TokenType::FORWARD_SLASH },
        { '%', TokenType::PERCENT },
        { '!', TokenType::EXCLAMATION_POINT },
        { '<', TokenType::LESS_THAN },
        { '>', TokenType::GREATER_THAN },
        { '=', TokenType::ASSIGNMENT }
    };

    m_double_char_tokens = {
        { "--", TokenType::DECREMENT },
        { "&&", TokenType::LOGICAL_AND },
        { "||", TokenType::LOGICAL_OR },
        { "==", TokenType::EQUAL },
        { "!=", TokenType::NOT_EQUAL },
        { "<=", TokenType::LESS_THAN_EQUAL },
        { ">=", TokenType::GREATER_THAN_EQUAL }
    };

    // Initialize regex patterns
    m_patterns = {
        { boost::regex("^[0-9]+\\b"), TokenType::CONSTANT },
        { boost::regex("^[a-zA-Z_]\\w*\\b"), TokenType::IDENTIFIER }
    };
}
