#pragma once
#include "common/data/source_location.h"
#include "common/data/token_table.h"
#include "common/data/type.h"
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
    using LiteralType = std::variant<std::monostate, ConstantType, std::string>;
    Token(TokenType type, const std::string& lexeme, LiteralType literal, const SourceLocation& source_location);
    std::string to_string() const;
    TokenType type() const { return m_type; }

    // For std::string
    template<typename T>
    requires std::is_same_v<T, std::string>
    T literal() const
    {
        if (std::holds_alternative<std::string>(m_literal)) {
            return std::get<std::string>(m_literal);
        }
        throw TokenError("Token doesn't contain string literal: " + to_string());
    }

    // For ConstantType cases
    template<typename T>
    requires(!std::is_same_v<T, std::string>)
    T literal() const
    {
        if (std::holds_alternative<ConstantType>(m_literal)) {
            const auto& constant_literal = std::get<ConstantType>(m_literal);
            if (std::holds_alternative<T>(constant_literal)) {
                return std::get<T>(constant_literal);
            }
        }
        throw TokenError("Token doesn't contain requested constant type: " + to_string());
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
