#include "common//data/source_manager.h"
#include "common/data/token_table.h"
#include "common/data/warning_manager.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "parser/parser_ast.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>

namespace fs = std::filesystem;
using namespace parser;

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Create a shared TokenTable for all tests
        token_table = std::make_shared<TokenTable>();
        source_manager = std::make_shared<SourceManager>();
        warning_manager = std::make_shared<WarningManager>();
        // Create a temporary directory for test files
        test_dir = fs::temp_directory_path() / "parser_tests";
        fs::create_directories(test_dir);
    }

    void TearDown() override
    {
        // Clean up test files
        fs::remove_all(test_dir);
    }

    // Helper function to create a test file with given content
    std::string create_test_file(const std::string& content, const std::string& filename = "test.i")
    {
        std::string filepath = (test_dir / filename).string();
        std::ofstream file(filepath);
        file << content;
        file.close();
        return filepath;
    }

    // Helper function to parse a string directly
    std::shared_ptr<Program> parse_string(const std::string& content)
    {
        std::string filepath = create_test_file(content);
        LexerContext lexer_context { filepath, token_table, source_manager, warning_manager };
        Lexer lexer(lexer_context);
        auto tokens = std::make_shared<std::vector<Token>>(lexer.tokenize());
        source_manager->set_token_list(tokens);
        Parser parser(*tokens, source_manager);
        return parser.parse_program();
    }

    // Helper to check if parsing throws an exception
    void expect_parse_error(const std::string& content)
    {
        std::string filepath = create_test_file(content);
        LexerContext lexer_context { filepath, token_table, source_manager, warning_manager };
        Lexer lexer(lexer_context);
        auto tokens = std::make_shared<std::vector<Token>>(lexer.tokenize());
        source_manager->set_token_list(tokens);
        Parser parser(*tokens, source_manager);
        EXPECT_THROW(parser.parse_program(), Parser::ParserError);
    }

    std::shared_ptr<TokenTable> token_table;
    std::shared_ptr<SourceManager> source_manager;
    std::shared_ptr<WarningManager> warning_manager;
    fs::path test_dir;
};