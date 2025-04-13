#pragma once
#include "common/data/token_table.h"
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>

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
    TokenType type() const {return m_type;}
    int line() const {return m_line;}

    template<typename T>
    T literal() const {
        if (!std::holds_alternative<T>(m_literal)) {
            throw TokenError("Bad Token" + to_string());
        }
        return std::get<T>(m_literal);
    }

    const std::string& lexeme() const {return m_lexeme;}
private:
    TokenType m_type;
    std::string m_lexeme;
    std::variant<std::monostate, int> m_literal;
    int m_line;
};
