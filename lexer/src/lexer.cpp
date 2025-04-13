#include "lexer/lexer.h"
#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

const std::string Lexer::file_extension = ".i";

Lexer::Lexer(const std::string& file_path)
{
    // Check if file exists
    if (!fs::exists(file_path)) {
        throw LexerError(std::format("Lexer Error file {} does not exist!", file_path));
    }

    // Check file extension
    std::string extension = fs::path(file_path).extension().string();
    if (extension != ".i") {
        throw LexerError(std::format("Lexer Error file has wrong extension. Expected: {}  Got: {}", file_extension, extension));
    }

    // Open the file
    std::ifstream file(file_path, std::ios::binary);
    if (!file.is_open()) {
        throw LexerError(std::format("Lexer Error failed to open file {}", file_path));
    }

    // Read the file content to string
    file.seekg(0, std::ios::end);
    m_file_content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&m_file_content[0], m_file_content.size());
    if (m_file_content.size() == 0) {
        throw LexerError(std::format("Lexer Error empty file {}", file_path));
    }
}

std::vector<Token> Lexer::tokenize()
{
    const std::string& input = m_file_content;
    std::vector<Token> res;
    size_t i = 0;
    TokenTable& tb = TokenTable::instance();
    size_t num_lines = 1;
    while (i < input.size()) {
        if (input[i] == ' ') {
            i++;
            continue;
        }

        if (input[i] == '\n') {
            i++;
            num_lines++;
            continue;
        }

        std::string_view curr_str(input.begin() + i, input.end());
        size_t search_res = tb.search(curr_str);
        if (search_res == 0) {
            throw LexerError(std::format("Lexer error at line: {}, no match found!", num_lines));
        }

        std::string lexeme = input.substr(i, search_res);
        try {
            Token t(lexeme, num_lines);
            res.push_back(t);
        } catch (const TokenError& e) {
            throw LexerError(std::format("Lexer error constructing token: {}", e.what()));
        }
        i += search_res;
    }
    return res;
}
