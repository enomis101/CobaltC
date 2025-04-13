#pragma once
#include "common/data/token.h"
#include <stdexcept>
#include "common/data/token_table.h"
#include <vector>

class LexerError : public std::runtime_error {
public:
    explicit LexerError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};



class Lexer {
public:
    Lexer(const std::string& file_path);
    std::vector<Token> tokenize();
private:
    static const std::string file_extension;
    std::string m_file_content;
};

