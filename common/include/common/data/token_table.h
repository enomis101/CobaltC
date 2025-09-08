#pragma once
#include <optional>
#include <regex>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class TokenType {
    IDENTIFIER,
    CONSTANT,
    LONG_CONSTANT,
    UNSIGNED_CONSTANT,
    UNSIGNED_LONG_CONSTANT,
    DOUBLE_CONSTANT,
    CHAR_LITERAL,
    STRING_LITERAL,
    INT_KW,
    LONG_KW,
    DOUBLE_KW,
    SIGNED_KW,
    UNSIGNED_KW,
    VOID_KW,
    CHAR_KW,
    RETURN_KW,
    IF_KW,
    ELSE_KW,
    DO_KW,
    WHILE_KW,
    FOR_KW,
    BREAK_KW,
    CONTINUE_KW,
    STATIC_KW,
    EXTERN_KW,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    OPEN_SQUARE_BRACKET,
    CLOSE_SQUARE_BRACKET,
    SEMICOLON,
    // UNARY_OPERATORS
    MINUS,
    DECREMENT,
    COMPLEMENT,
    EXCLAMATION_POINT, // !
    // MIX
    AMPERSAND,
    ASTERISK,
    // BINARY_OPERATORS
    PLUS,
    FORWARD_SLASH,
    PERCENT,
    // LOGICAL_OPERATORS
    LOGICAL_AND, // &&
    LOGICAL_OR,  // ||
    // COMPARISON_OPERATORS
    EQUAL,              // ==
    NOT_EQUAL,          // !=
    LESS_THAN,          // <
    GREATER_THAN,       // >
    LESS_THAN_EQUAL,    // <=
    GREATER_THAN_EQUAL, // >=
    ASSIGNMENT,         // =
    // CONDITIONAL OPERATORS
    QUESTION_MARK,
    COLON,
    // OTHERS
    COMMA
    // Add other types as needed
};
class TokenTable {
public:
    TokenTable();

    // Delete copy
    TokenTable(const TokenTable&) = delete;
    TokenTable& operator=(const TokenTable&) = delete;
    TokenTable(TokenTable&&) = delete;
    TokenTable& operator=(TokenTable&&) = delete;

    // Returns the length of the match at the beginning of input, or 0 if no match
    size_t search(std::string_view input) const;

    // Determines the type of a lexeme (for token classification)
    std::optional<TokenType> match(std::string_view lexeme) const;

private:
    // Data members
    std::unordered_map<std::string_view, TokenType> m_keywords;
    std::vector<std::pair<std::regex, TokenType>> m_constant_search_patterns;
    std::vector<std::pair<std::regex, TokenType>> m_constant_match_patterns;
    std::vector<std::pair<std::regex, TokenType>> m_literal_patterns;
    std::unordered_map<char, TokenType> m_single_char_tokens;
    std::unordered_map<std::string_view, TokenType> m_double_char_tokens;
    std::regex m_identifier_pattern;
};
