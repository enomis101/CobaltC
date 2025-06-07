#include "lexer/lexer.h"
#include "common/data/source_location.h"
#include "common/data/source_manager.h"
#include "common/data/token_table.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <regex>
#include <string>

namespace fs = std::filesystem;

const std::string Lexer::file_extension = ".i";

Lexer::Lexer(const std::string& file_path, std::shared_ptr<TokenTable> token_table)
    : m_file_path { file_path }
    , m_token_table { token_table }
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

    LocationTracker curr_location_tracker(m_file_path);
    static const std::regex line_directive_pattern("^#\\s*(\\d+)\\s+\"([^\"]*)\"\\s*(.*?)$");

    while (i < input.size()) {
        // Skip whitespace but track line numbers and columns
        if (input[i] == ' ' || input[i] == '\t') {
            i++;
            curr_location_tracker.advance();
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
                curr_location_tracker.reset(matches[2].str(), line_num);
            } catch (std::exception& e) {
                throw LexerError(std::format("Failed parsing line directive: {}", e.what()));
            }
        }

        if (input[i] == '\n') {
            i++;
            curr_location_tracker.new_line();
            continue;
        }

        std::string_view curr_str(input.begin() + i, input.end());
        size_t search_res = m_token_table->search(curr_str);
        if (search_res == 0) {
            auto err = SourceManager::get_source_line(curr_location_tracker.current());
            assert(err.has_value());
            throw LexerError(std::format("Failed matching a token \n{}", (err.has_value() ? err.value() : "NOT FOUND!")));
        }

        std::string lexeme = input.substr(i, search_res);
        std::optional<TokenType> result = m_token_table->match(lexeme);

        assert(result.has_value() && "TokenTable::match failed after valid search!");

        TokenType type = result.value();
        Token::LiteralType literal;

        if (type == TokenType::CONSTANT) {
            // With error handling
            try {
                int num = std::stoi(lexeme); // This will throw an exception
                literal = num;
            } catch (const std::exception& e) {
                throw LexerError(std::format("Error while parsing constant in Token constructor: {}", e.what()));
            }
        }

        Token t(type, lexeme, literal, curr_location_tracker.current());
        res.push_back(t);

        curr_location_tracker.advance(search_res);
        i += search_res;
    }
    return res;
}
