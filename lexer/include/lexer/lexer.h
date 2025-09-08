#pragma once
#include "common/data/source_location.h"
#include "common/data/source_manager.h"
#include "common/data/token.h"
#include "common/data/token_table.h"
#include "common/data/warning_manager.h"
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

struct LexerContext {
    std::string file_path;
    std::shared_ptr<TokenTable> token_table;
    std::shared_ptr<SourceManager> source_manager;
    std::shared_ptr<WarningManager> warning_manager;
};

class Lexer {
public:
    Lexer(const LexerContext& lexer_context);
    std::vector<Token> tokenize();

private:
    static const std::string file_extension;
    std::string m_file_content;
    std::string m_file_path;
    std::shared_ptr<TokenTable> m_token_table;
    std::shared_ptr<SourceManager> m_source_manager;
    std::shared_ptr<WarningManager> m_warning_manager;
    LocationTracker m_curr_location_tracker;

    std::pair<TokenType, Token::LiteralType> convert_literal_value(const std::string& lexeme, TokenType type);
    double parse_double(const std::string& lexeme);
    std::string unescape(const std::string& str);
    bool is_literal(TokenType type);
    bool is_valid_escape_sequence(char c, char& escape_sequence);
    bool get_escape_sequence(char c);
};
