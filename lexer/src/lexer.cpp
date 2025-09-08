#include "lexer/lexer.h"
#include "common/data/source_location.h"
#include "common/data/token_table.h"
#include "common/data/type.h"
#include "common/data/warning_manager.h"
#include "common/error/internal_compiler_error.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <utility>

#include <charconv>
#include <string_view>
#include <variant>

namespace fs = std::filesystem;

const std::string Lexer::file_extension = ".i";

Lexer::Lexer(const LexerContext& lexer_context)
    : m_file_path { lexer_context.file_path }
    , m_token_table { lexer_context.token_table }
    , m_source_manager { lexer_context.source_manager }
    , m_warning_manager(lexer_context.warning_manager)
    , m_curr_location_tracker { m_file_path }
{
    // Check if file exists
    if (!fs::exists(m_file_path)) {
        throw LexerError(std::format("File not found: '{}' - Please check the path and try again", m_file_path));
    }

    // Check file extension
    std::string extension = fs::path(m_file_path).extension().string();
    if (extension != file_extension) {
        throw LexerError(std::format(
            "Invalid file extension: Expected '{}' but got '{}' - Preprocessed files must have '{}' extension",
            file_extension, extension, file_extension));
    }

    // Open the file
    std::ifstream file(m_file_path, std::ios::binary);
    if (!file.is_open()) {
        throw LexerError(std::format(
            "Failed to open file '{}' - Check file permissions and if the file is in use",
            m_file_path));
    }

    // Read the file content to string
    file.seekg(0, std::ios::end);
    m_file_content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&m_file_content[0], m_file_content.size());
    if (m_file_content.empty()) {
        throw LexerError(std::format(
            "Empty file: '{}' - Input file contains no content to tokenize",
            m_file_path));
    }
}

std::vector<Token> Lexer::tokenize()
{
    const std::string& input = m_file_content;
    std::vector<Token> res;
    size_t i = 0;

    static const std::regex line_directive_pattern("^#\\s*(\\d+)\\s+\"([^\"]*)\"\\s*(.*?)$");

    while (i < input.size()) {
        // Skip whitespace but track line numbers and columns
        if (input[i] == ' ' || input[i] == '\t') {
            i++;
            m_curr_location_tracker.advance();
            continue;
        }

        if (input[i] == '#') {
            size_t j = i;
            while (input[j] != '\n') {
                j++;
                if (j >= input.size()) {
                    throw LexerError(("Unexpected EOF"));
                }
            }
            std::string line = input.substr(i, j - i + 1);
            std::smatch matches;

            if (!std::regex_match(line, matches, line_directive_pattern)) {
                throw LexerError(("Line starting with # does not match a line directive pattern"));
            }
            try {
                int line_num = std::stoi(matches[1].str());
                m_curr_location_tracker.reset(matches[2].str(), line_num);
            } catch (std::exception& e) {
                throw LexerError(std::format("Failed parsing line directive: {}", e.what()));
            }
        }

        if (input[i] == '\n') {
            i++;
            m_curr_location_tracker.new_line();
            continue;
        }

        std::string_view curr_str(input.begin() + i, input.end());
        size_t search_res = m_token_table->search(curr_str);
        if (search_res == 0) {
            auto err = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Failed matching a token \n{}", err));
        }

        std::string lexeme = input.substr(i, search_res);
        std::optional<TokenType> result = m_token_table->match(lexeme);

        if (!result.has_value()) {
            auto err = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw InternalCompilerError(std::format("TokenTable::match failed after valid search!\n{}", err));
        }

        TokenType type = result.value();
        Token::LiteralType literal;

        if (is_literal(type)) {
            try {
                std::tie(type, literal) = convert_literal_value(lexeme, type);
            } catch (std::exception& e) {
                auto err = m_source_manager->get_source_line(m_curr_location_tracker.current());
                throw InternalCompilerError(std::format("TokenTable::match failed convert_literal_value\n{}\n{}", std::string(e.what()), err));
            }
        }

        Token t(type, lexeme, literal, m_curr_location_tracker.current());
        res.push_back(t);

        m_curr_location_tracker.advance(search_res);
        i += search_res;
    }
    return res;
}

std::pair<TokenType, Token::LiteralType> Lexer::convert_literal_value(const std::string& lexeme, const TokenType type)
{
    TokenType new_type = type;
    Token::LiteralType new_literal;
    ConstantType constant_literal;
    std::string string_literal;
    if (type == TokenType::CONSTANT) {
        try {
            // Try int first
            size_t pos;
            long long_val = std::stol(lexeme, &pos);

            if (pos != lexeme.length()) {
                throw std::invalid_argument("Invalid format");
            }

            if (long_val >= INT_MIN && long_val <= INT_MAX) {
                constant_literal = static_cast<int>(long_val);
            } else {
                /* If a constant is too large to store as an int,
                 * it's automatically promoted to long, even without an 'L' suffix
                 */
                auto warn_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
                m_warning_manager->raise_warning(LexerWarningType::CAST, std::format("Integer constant '{}' exceeds int range [{}, {}], automatically promoting to long:\n{}", lexeme, INT_MIN, INT_MAX, warn_line));
                new_type = TokenType::LONG_CONSTANT;
                constant_literal = long_val;
            }
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing integer constant '{}' {} at:\n{}", lexeme, e.what(), err_line));
        }
    } else if (type == TokenType::UNSIGNED_CONSTANT) {
        try {
            // Try unsigned int first
            size_t pos;
            unsigned long ulong_val = std::stoul(lexeme.substr(0, lexeme.size() - 1), &pos); // Remove 'U' suffix

            if (pos != lexeme.length() - 1) {
                throw std::invalid_argument("Invalid format");
            }

            if (ulong_val <= UINT_MAX) {
                constant_literal = static_cast<unsigned int>(ulong_val);
            } else {
                /* If an unsigned constant is too large to store as unsigned int,
                 * it's automatically promoted to unsigned long, even with just 'U' suffix
                 */
                auto warn_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
                m_warning_manager->raise_warning(LexerWarningType::CAST, std::format("Unsigned constant '{}' exceeds unsigned int range [0, {}], automatically promoting to unsigned long:\n{}", lexeme, UINT_MAX, warn_line));
                new_type = TokenType::UNSIGNED_LONG_CONSTANT;
                constant_literal = ulong_val;
            }
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing unsigned constant '{} {}' at:\n{}", lexeme, e.what(), err_line));
        }
    } else if (type == TokenType::LONG_CONSTANT) {
        try {
            // Remove 'L' or 'l' suffix before parsing
            std::string numeric_part = lexeme.substr(0, lexeme.size() - 1);
            long num = std::stol(numeric_part);
            constant_literal = num;
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing long constant '{}' {} at:\n{}", lexeme, e.what(), err_line));
        }
    } else if (type == TokenType::UNSIGNED_LONG_CONSTANT) {
        try {
            // Remove 'UL', 'ul', 'LU', or 'lu' suffix before parsing
            std::string numeric_part = lexeme;
            // Remove up to 2 suffix characters (U/u and L/l in any order)
            if (numeric_part.size() >= 2) {
                char last = std::tolower(numeric_part.back());
                char second_last = std::tolower(numeric_part[numeric_part.size() - 2]);

                if ((last == 'l' && second_last == 'u') || (last == 'u' && second_last == 'l')) {
                    numeric_part = numeric_part.substr(0, numeric_part.size() - 2);
                } else if (last == 'l' || last == 'u') {
                    numeric_part = numeric_part.substr(0, numeric_part.size() - 1);
                }
            }

            unsigned long num = std::stoul(numeric_part);
            constant_literal = num;
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing unsigned long constant '{}' {} at:\n{}", lexeme, e.what(), err_line));
        }
    } else if (type == TokenType::DOUBLE_CONSTANT) {
        try {
            double num = parse_double(lexeme);
            constant_literal = num;
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing double constant '{}' {} at:\n{}", lexeme, e.what(), err_line));
        }
    } else if (type == TokenType::CHAR_LITERAL) {
        std::string unescaped_string = unescape(lexeme);
        // empty character constants are not valid
        if (unescaped_string.size() != 3) {
            throw LexerError("Error while parsing char literal!");
        }
        if (unescaped_string[0] != '\'' || unescaped_string.back() != '\'') {
            throw LexerError("Error while parsing char literal!");
        }
        int c = unescaped_string[1];
        constant_literal = c;
    } else if (type == TokenType::STRING_LITERAL) {
        std::string unescaped_string = unescape(lexeme);
        if (unescaped_string.size() < 2) {
            throw LexerError("Error while parsing string literal!");
        }
        if (unescaped_string[0] != '\"' || unescaped_string.back() != '\"') {
            throw LexerError("Error while parsing string literal!");
        }
        string_literal = unescaped_string.substr(1, unescaped_string.size() - 2);
    }

    if (constant_literal.index() > 0) {
        new_literal = constant_literal;
    } else {
        new_literal = string_literal;
    }

    return { new_type, new_literal };
}

bool Lexer::is_literal(TokenType type)
{
    switch (type) {
    case TokenType::CHAR_LITERAL:
    case TokenType::STRING_LITERAL:
    case TokenType::CONSTANT:
    case TokenType::LONG_CONSTANT:
    case TokenType::UNSIGNED_CONSTANT:
    case TokenType::UNSIGNED_LONG_CONSTANT:
    case TokenType::DOUBLE_CONSTANT:
        return true;
    default:
        return false;
    }
}

std::string Lexer::unescape(const std::string& str)
{
    std::string new_string;
    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];
        if (c == '\\') {
            size_t j = i + 1;
            if (j >= str.size()) {
                throw LexerError("Invalid escape sequence, expected character after backslash");
            }
            char escape_sequence;
            bool res = is_valid_escape_sequence(str[j], escape_sequence);
            if (!res) {
                throw LexerError("Invalid escape sequence");
            }
            new_string.push_back(escape_sequence);
            ++i;
        } else {
            new_string.push_back(c);
        }
    }
    return new_string;
}

bool Lexer::is_valid_escape_sequence(char c, char& escape_sequence)
{
    switch (c) {
    case '\'': {
        escape_sequence = '\'';
        return true;
    }
    case '"': {
        escape_sequence = '\"';
        return true;
    }
    case '?': {
        escape_sequence = '\?';
        return true;
    }
    case '\\': {
        escape_sequence = '\\';
        return true;
    }
    case 'a': {
        escape_sequence = '\a';
        return true;
    }
    case 'b': {
        escape_sequence = '\b';
        return true;
    }
    case 'f': {
        escape_sequence = '\f';
        return true;
    }
    case 'n': {
        escape_sequence = '\n';
        return true;
    }
    case 'r': {
        escape_sequence = '\r';
        return true;
    }
    case 't': {
        escape_sequence = '\t';
        return true;
    }
    case 'v': {
        escape_sequence = '\v';
        return true;
    }
    default:
        return false;
    }
}

double Lexer::parse_double(const std::string& lexeme)
{
    char* end;
    errno = 0; // Clear errno before calling strtod
    double result = std::strtod(lexeme.c_str(), &end);

    // Check if the entire string was consumed
    if (end != lexeme.c_str() + lexeme.size()) {
        throw std::invalid_argument("Invalid number format");
    }

    // strtod handles overflow/underflow according to IEEE 754:
    // - Overflow: returns ±HUGE_VAL (±infinity) and sets errno to ERANGE
    // - Underflow: returns 0.0 and sets errno to ERANGE
    // - Normal: returns the value and errno remains 0

    return result;
}
