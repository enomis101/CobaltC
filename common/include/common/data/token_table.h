#pragma once
#include <boost/regex.hpp>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class TokenType {
    IDENTIFIER,
    CONSTANT,
    LONG_CONSTANT,
    INT_KW,
    VOID_KW,
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
    LONG_KW,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    SEMICOLON,
    // UNARY_OPERATORS
    MINUS,
    DECREMENT,
    COMPLEMENT,
    EXCLAMATION_POINT, // !
    // BINARY_OPERATORS
    PLUS,
    ASTERISK,
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
    std::vector<std::pair<boost::regex, TokenType>> m_patterns;
    std::unordered_map<char, TokenType> m_single_char_tokens;
    std::unordered_map<std::string_view, TokenType> m_double_char_tokens;
};
