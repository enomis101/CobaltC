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

    for (const auto& p : m_constant_search_patterns) {
        const std::regex& pattern = p.first;
        std::match_results<std::string_view::const_iterator> matches;
        if (std::regex_search(input.begin(), input.end(), matches, pattern)) {
            // Return the length of the matched text
            if(matches.size() > 1){
                std::string_view first_group(matches[1].first, matches[1].second);
                return first_group.size();
            }
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

    for (const auto& p : m_constant_match_patterns) {
        const std::regex& pattern = p.first;
        TokenType type = p.second;
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
        {"double", TokenType::DOUBLE_KW}
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
        { ',', TokenType::COMMA }
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

    std::vector<std::string> base_patterns = {"^([0-9]+)", "^([0-9]+[lL])", "^([0-9]+[uU])", "^([0-9]+([uU][lL]|[lL][uU]))", "^((([0-9]*\\.[0-9]+|[0-9]+\\.?)[Ee][+-]?[0-9]+|[0-9]*\\.[0-9]+|[0-9]+\\.))"};
    // Initialize regex patterns
    m_constant_search_patterns = {
        { std::regex(base_patterns[0] + "[^\\w.]"), TokenType::CONSTANT },
        { std::regex(base_patterns[1] + "[^\\w.]"), TokenType::LONG_CONSTANT },
        { std::regex(base_patterns[2] + "[^\\w.]"), TokenType::UNSIGNED_CONSTANT },
        { std::regex(base_patterns[3] + "[^\\w.]"), TokenType::UNSIGNED_LONG_CONSTANT },
        {
             std::regex(base_patterns[4] + "[^\\w.]"), TokenType::DOUBLE_CONSTANT}
    };

    m_constant_match_patterns = {
        { std::regex(base_patterns[0] + "$"), TokenType::CONSTANT },
        { std::regex(base_patterns[1] + "$"), TokenType::LONG_CONSTANT },
        { std::regex(base_patterns[2] + "$"), TokenType::UNSIGNED_CONSTANT },
        { std::regex(base_patterns[3] + "$"), TokenType::UNSIGNED_LONG_CONSTANT },
        { std::regex(base_patterns[4] + "$"), TokenType::DOUBLE_CONSTANT}
    };

    m_identifier_pattern = std::regex("^([a-zA-Z_]\\w*\\b)");
}
