#pragma once
#include <string>
#include <variant>

enum class TokenType {

};

class Token {
public:
private:
    TokenType m_type;
    std::string m_lexeme;
    std::variant<std::monostate, std::string, double> m_literal;
};
