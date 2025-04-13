#pragma once
#include <boost/regex.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

enum class TokenType {
    IDENTIFIER,
    CONSTANT,
    INT_KW,
    VOID_KW,
    RETURN_KW,
    OPEN_PAREN,
    CLOSE_PAREN,
    OPEN_BRACE,
    CLOSE_BRACE,
    SEMICOLON
    // Add other types as needed
};

class TokenTable {
public:
    // Delete copy and move operations to enforce singleton semantics
    TokenTable(const TokenTable&) = delete;
    TokenTable& operator=(const TokenTable&) = delete;
    TokenTable(TokenTable&&) = delete;
    TokenTable& operator=(TokenTable&&) = delete;

    // Static method to access the singleton instance
    static TokenTable& instance();

    // Returns the length of the match at the beginning of input, or 0 if no match
    size_t search(std::string_view input) const;

    // Determines the type of a lexeme (for token classification)
    std::optional<TokenType> match(std::string_view lexeme) const;

private:
    // Private constructor - can only be called from getInstance()
    TokenTable();

    // Data members
    std::unordered_map<std::string_view, TokenType> m_keywords;
    std::vector<std::pair<boost::regex, TokenType>> m_patterns;
    std::unordered_map<char, TokenType> m_single_char_tokens;
};
