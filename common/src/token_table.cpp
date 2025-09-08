#include "common/data/token_table.h"
#include <vector>

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

    for (const auto& [pattern, _] : m_constant_search_patterns) {
        std::match_results<std::string_view::const_iterator> matches;
        if (std::regex_search(input.begin(), input.end(), matches, pattern)) {
            // Return the length of the matched text
            if (matches.size() > 1) {
                std::string_view first_group(matches[1].first, matches[1].second);
                return first_group.size();
            }
        }
    }

    for (const auto& [pattern, _] : m_literal_patterns) {
        std::match_results<std::string_view::const_iterator> matches;
        if (std::regex_search(input.begin(), input.end(), matches, pattern)) {
            // Return the length of the matched text
            return matches.length();
        }
    }

    std::match_results<std::string_view::const_iterator> matches;
    if (std::regex_search(input.begin(), input.end(), matches, m_identifier_pattern)) {
        // Return the length of the matched text
        return matches.length();
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

    for (const auto& [pattern, type] : m_constant_match_patterns) {
        std::match_results<std::string_view::const_iterator> matches;
        if (std::regex_match(lexeme.begin(), lexeme.end(), matches, pattern)) {
            res = type;
            return res;
        }
    }

    for (const auto& [pattern, type] : m_literal_patterns) {
        std::match_results<std::string_view::const_iterator> matches;
        if (std::regex_match(lexeme.begin(), lexeme.end(), matches, pattern)) {
            res = type;
            return res;
        }
    }

    std::match_results<std::string_view::const_iterator> matches;
    if (std::regex_match(lexeme.begin(), lexeme.end(), matches, m_identifier_pattern)) {
        res = TokenType::IDENTIFIER;
        if (m_keywords.contains(lexeme)) {
            res = m_keywords.at(lexeme);
        }
        return res;
    }

    return res;
}

TokenTable::TokenTable()
{
    // Initialize keywords
    m_keywords = {
        { "int", TokenType::INT_KW },
        { "void", TokenType::VOID_KW },
        { "return", TokenType::RETURN_KW },
        { "if", TokenType::IF_KW },
        { "else", TokenType::ELSE_KW },
        { "do", TokenType::DO_KW },
        { "while", TokenType::WHILE_KW },
        { "for", TokenType::FOR_KW },
        { "break", TokenType::BREAK_KW },
        { "continue", TokenType::CONTINUE_KW },
        { "static", TokenType::STATIC_KW },
        { "extern", TokenType::EXTERN_KW },
        { "long", TokenType::LONG_KW },
        { "signed", TokenType::SIGNED_KW },
        { "unsigned", TokenType::UNSIGNED_KW },
        { "double", TokenType::DOUBLE_KW },
        { "char", TokenType::CHAR_KW }
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
        { '=', TokenType::ASSIGNMENT },
        { '?', TokenType::QUESTION_MARK },
        { ':', TokenType::COLON },
        { ',', TokenType::COMMA },
        { '&', TokenType::AMPERSAND },
        { '[', TokenType::OPEN_SQUARE_BRACKET },
        { ']', TokenType::CLOSE_SQUARE_BRACKET }
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

    // REMEMBER: add ^ anchor at the beginning of the string
    // The ^ anchor ensures that regex_search() matches from the beginning of the input
    m_literal_patterns = {
        { std::regex(R"(^'([^'\\\n]|\\['"?\\abfnrtv])')"), TokenType::CHAR_LITERAL },
        { std::regex(R"(^"([^"\\\n]|\\['"?\\abfnrtv])*")"), TokenType::STRING_LITERAL },
    };

    std::vector<std::pair<std::string, TokenType>> base_patterns = {
        { "^([0-9]+)", TokenType::CONSTANT },
        { "^([0-9]+[lL])", TokenType::LONG_CONSTANT },
        { "^([0-9]+[uU])", TokenType::UNSIGNED_CONSTANT },
        { "^([0-9]+([uU][lL]|[lL][uU]))", TokenType::UNSIGNED_LONG_CONSTANT },
        { "^((([0-9]*\\.[0-9]+|[0-9]+\\.?)[Ee][+-]?[0-9]+|[0-9]*\\.[0-9]+|[0-9]+\\.))", TokenType::DOUBLE_CONSTANT },
    };

    for (const auto& [base_pattern, type] : base_patterns) {
        m_constant_search_patterns.push_back({ std::regex(base_pattern + "[^\\w.]"), type });
        m_constant_match_patterns.push_back({ std::regex(base_pattern + "$"), type });
    }

    m_identifier_pattern = std::regex("^([a-zA-Z_]\\w*\\b)");
}
