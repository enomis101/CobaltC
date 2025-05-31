#pragma once
#include "common/data/token.h"
#include "common/data/token_table.h"
#include <memory>
#include <stdexcept>
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
    Lexer(const std::string& file_path, std::shared_ptr<TokenTable> token_table);
    std::vector<Token> tokenize();

private:
    static const std::string file_extension;
    std::string m_file_content;
    std::shared_ptr<TokenTable> m_token_table;
};
