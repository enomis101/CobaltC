#pragma once
#include <string>
#include <variant>
#include <unordered_map>
#include <stdexcept>
#include "common/data/token_table.h"

class TokenError : public std::runtime_error {
public:
    explicit TokenError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class Token {
public:
    Token(const std::string& lexeme, int line);
    std::string to_string() const; 
private:
    TokenType m_type;
    std::string m_lexeme;
    std::variant<std::monostate, int> m_literal;
    int m_line;
};
