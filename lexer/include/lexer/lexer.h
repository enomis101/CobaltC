#pragma once
#include "common/data/source_location.h"
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

class LocationTracker {
public:
    explicit LocationTracker(const SourceLocation& initial_location)
        : m_source_location(initial_location)
    {
    }

    void reset(const std::string& new_file_name, size_t new_line_num)
    {
        m_source_location.file_name = new_file_name;
        m_source_location.line_number = new_line_num;
        m_source_location.column_number = 1;
    }

    void advance(size_t count = 1)
    {
        m_source_location.column_number += count;
    }

    void new_line()
    {
        m_source_location.line_number++;
        m_source_location.column_number = 1;
    }

    SourceLocation current() const
    {
        return m_source_location;
    }

private:
    SourceLocation m_source_location;
};

class Lexer {
public:
    Lexer(const std::string& file_path, std::shared_ptr<TokenTable> token_table);
    std::vector<Token> tokenize();

private:
    static const std::string file_extension;
    std::string m_file_content;
    std::string m_file_path;
    std::shared_ptr<TokenTable> m_token_table;
};
