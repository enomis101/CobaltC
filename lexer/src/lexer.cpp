#include "lexer/lexer.h"
#include "common/data/source_location.h"
#include "common/data/token_table.h"
#include "common/log/log.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

namespace fs = std::filesystem;

const std::string Lexer::file_extension = ".i";

Lexer::Lexer(const std::string& file_path, std::shared_ptr<TokenTable> token_table, std::shared_ptr<SourceManager> source_manager)
    : m_file_path { file_path }
    , m_token_table { token_table }
    , m_source_manager { source_manager }
    , m_curr_location_tracker { m_file_path }
{
    // Check if file exists
    if (!fs::exists(file_path)) {
        throw LexerError(std::format("File not found: '{}' - Please check the path and try again", file_path));
    }

    // Check file extension
    std::string extension = fs::path(file_path).extension().string();
    if (extension != file_extension) {
        throw LexerError(std::format(
            "Invalid file extension: Expected '{}' but got '{}' - Preprocessed files must have '{}' extension",
            file_extension, extension, file_extension));
    }

    // Open the file
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw LexerError(std::format(
            "Failed to open file '{}' - Check file permissions and if the file is in use",
            file_path));
    }

    // Read the file content to string
    file.seekg(0, std::ios::end);
    m_file_content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&m_file_content[0], m_file_content.size());
    if (m_file_content.empty()) {
        throw LexerError(std::format(
            "Empty file: '{}' - Input file contains no content to tokenize",
            file_path));
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

        assert(result.has_value() && "TokenTable::match failed after valid search!");

        TokenType type = result.value();
        Token::LiteralType literal;

        auto [new_type, new_literal] = convert_constant(lexeme, type, literal);

        Token t(new_type, lexeme, new_literal, m_curr_location_tracker.current());
        res.push_back(t);

        m_curr_location_tracker.advance(search_res);
        i += search_res;
    }
    return res;
}

std::pair<TokenType, Token::LiteralType> Lexer::convert_constant(const std::string& lexeme, const TokenType type, const Token::LiteralType literal)
{
    TokenType new_type = type;
    Token::LiteralType new_literal = literal;
    if (type == TokenType::CONSTANT) {
        try {
            // Try int first
            size_t pos;
            long long_val = std::stol(lexeme, &pos);

            if (pos != lexeme.length()) {
                throw std::invalid_argument("Invalid format");
            }

            if (long_val >= INT_MIN && long_val <= INT_MAX) {
                new_literal = static_cast<int>(long_val);
            } else {
                /* If a constant is too large to store as an int,
                 * it's automatically promoted to long, even without an 'L' suffix
                 */
                auto warn_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
                LOG_WARN(LOG_CONTEXT, std::format("Integer constant '{}' exceeds int range [{}, {}], automatically promoting to long:\n{}", lexeme, INT_MIN, INT_MAX, warn_line));
                new_type = TokenType::LONG_CONSTANT;
                new_literal = long_val;
            }
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing integer constant '{}' at:\n{}", lexeme, err_line));
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
                new_literal = static_cast<unsigned int>(ulong_val);
            } else {
                /* If an unsigned constant is too large to store as unsigned int,
                 * it's automatically promoted to unsigned long, even with just 'U' suffix
                 */
                auto warn_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
                LOG_WARN(LOG_CONTEXT, std::format("Unsigned constant '{}' exceeds unsigned int range [0, {}], automatically promoting to unsigned long:\n{}", lexeme, UINT_MAX, warn_line));
                new_type = TokenType::UNSIGNED_LONG_CONSTANT;
                new_literal = ulong_val;
            }
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing unsigned constant '{}' at:\n{}", lexeme, err_line));
        }
    } else if (type == TokenType::LONG_CONSTANT) {
        try {
            // Remove 'L' or 'l' suffix before parsing
            std::string numeric_part = lexeme.substr(0, lexeme.size() - 1);
            long num = std::stol(numeric_part);
            new_literal = num;
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing long constant '{}' at:\n{}", lexeme, err_line));
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
            new_literal = num;
        } catch (const std::exception& e) {
            auto err_line = m_source_manager->get_source_line(m_curr_location_tracker.current());
            throw LexerError(std::format("Error parsing unsigned long constant '{}' at:\n{}", lexeme, err_line));
        }
    }
    return { new_type, new_literal };
}
