#pragma once
#include "common/data/source_location.h"
#include "common/data/token_table.h"
#include <stdexcept>
#include <string>
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
    using LiteralType = std::variant<std::monostate, int, long>;
    Token(TokenType type, const std::string& lexeme, LiteralType literal, const SourceLocation& source_location);
    std::string to_string() const;
    TokenType type() const { return m_type; }

    template<typename T>
    T literal() const
    {
        if (!std::holds_alternative<T>(m_literal)) {
            throw TokenError("Bad Token" + to_string());
        }
        return std::get<T>(m_literal);
    }

    const std::string& lexeme() const { return m_lexeme; }

    static std::string type_to_string(TokenType type);

    const SourceLocation& source_location() const { return m_source_location; }

private:
    TokenType m_type;
    std::string m_lexeme;
    LiteralType m_literal;
    SourceLocation m_source_location;
};
