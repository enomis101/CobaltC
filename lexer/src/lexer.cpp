#include "lexer/lexer.h"
#include "common/data/token_table.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <string>
#include <variant>

namespace fs = std::filesystem;

const std::string Lexer::file_extension = ".i";

Lexer::Lexer(const std::string& file_path, std::shared_ptr<TokenTable> token_table)
    : m_token_table { token_table }
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
    size_t line_num = 1;
    size_t col_num = 1;
    size_t line_start = 0; // Track start of current line

    while (i < input.size()) {
        // Skip whitespace but track line numbers and columns
        if (input[i] == ' ' || input[i] == '\t') {
            i++;
            col_num++;
            continue;
        }

        if (input[i] == '\n') {
            i++;
            line_num++;
            col_num = 1;
            line_start = i;
            continue;
        }

        std::string_view curr_str(input.begin() + i, input.end());
        size_t search_res = m_token_table->search(curr_str);
        if (search_res == 0) {
            // Create a context snippet showing the error location
            std::string line_snippet;
            size_t snippet_start = line_start;
            size_t snippet_end = i;

            // Find end of the current line
            while (snippet_end < input.size() && input[snippet_end] != '\n') {
                snippet_end++;
            }

            // Extract the line for context
            line_snippet = input.substr(snippet_start, snippet_end - snippet_start);

            // Create pointer to the error position
            std::string error_pointer(col_num - 1, ' ');
            error_pointer += "^";

            throw LexerError(std::format(
                "Lexical error at line {} column {}:\n"
                "{}\n"
                "{}\n"
                "Unrecognized token starting with '{}'",
                line_num, col_num,
                line_snippet,
                error_pointer,
                input[i]));
        }

        std::string lexeme = input.substr(i, search_res);
        std::optional<TokenType> result = m_token_table->match(lexeme);
        if (!result.has_value()) {

            throw LexerError(std::format("TokenTable::match failed after valid search!"));
        }

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

        Token t(type, lexeme, literal, line_num);
        res.push_back(t);
        i += search_res;
        col_num += search_res;
    }
    return res;
}
