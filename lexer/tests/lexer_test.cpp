#include "common/data/token_table.h"
#include "lexer/lexer.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>

namespace fs = std::filesystem;

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Create a shared TokenTable for all tests
        token_table = std::make_shared<TokenTable>();

        // Create a temporary directory for test files
        test_dir = fs::temp_directory_path() / "lexer_tests";
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

    std::shared_ptr<TokenTable> token_table;
    fs::path test_dir;
};

TEST_F(LexerTest, FileNotFound)
{
    std::string nonexistent_file = (test_dir / "nonexistent.i").string();

    EXPECT_THROW({ Lexer lexer(nonexistent_file, token_table); }, LexerError);
}

TEST_F(LexerTest, InvalidFileExtension)
{
    std::string wrong_extension_file = create_test_file("int main() {}", "test.c");

    EXPECT_THROW({ Lexer lexer(wrong_extension_file, token_table); }, LexerError);
}

TEST_F(LexerTest, EmptyFile)
{
    std::string empty_file = create_test_file("");

    EXPECT_THROW({ Lexer lexer(empty_file, token_table); }, LexerError);
}

TEST_F(LexerTest, SimpleIntegerConstant)
{
    std::string filepath = create_test_file("42");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "42");
    EXPECT_EQ(tokens[0].literal<int>(), 42);
    EXPECT_EQ(tokens[0].source_location().line_number, 1);
}

TEST_F(LexerTest, SimpleIdentifier)
{
    std::string filepath = create_test_file("myVariable");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme(), "myVariable");
    EXPECT_EQ(tokens[0].source_location().line_number, 1);
}

TEST_F(LexerTest, Keywords)
{
    std::string filepath = create_test_file("int return void if else while for");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type(), TokenType::INT_KW);
    EXPECT_EQ(tokens[1].type(), TokenType::RETURN_KW);
    EXPECT_EQ(tokens[2].type(), TokenType::VOID_KW);
    EXPECT_EQ(tokens[3].type(), TokenType::IF_KW);
    EXPECT_EQ(tokens[4].type(), TokenType::ELSE_KW);
    EXPECT_EQ(tokens[5].type(), TokenType::WHILE_KW);
    EXPECT_EQ(tokens[6].type(), TokenType::FOR_KW);
}

TEST_F(LexerTest, SingleCharacterTokens)
{
    std::string filepath = create_test_file("(){}+-*/%;,");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 11);
    EXPECT_EQ(tokens[0].type(), TokenType::OPEN_PAREN);
    EXPECT_EQ(tokens[1].type(), TokenType::CLOSE_PAREN);
    EXPECT_EQ(tokens[2].type(), TokenType::OPEN_BRACE);
    EXPECT_EQ(tokens[3].type(), TokenType::CLOSE_BRACE);
    EXPECT_EQ(tokens[4].type(), TokenType::PLUS);
    EXPECT_EQ(tokens[5].type(), TokenType::MINUS);
    EXPECT_EQ(tokens[6].type(), TokenType::ASTERISK);
    EXPECT_EQ(tokens[7].type(), TokenType::FORWARD_SLASH);
    EXPECT_EQ(tokens[8].type(), TokenType::PERCENT);
    EXPECT_EQ(tokens[9].type(), TokenType::SEMICOLON);
    EXPECT_EQ(tokens[10].type(), TokenType::COMMA);
}

TEST_F(LexerTest, DoubleCharacterTokens)
{
    std::string filepath = create_test_file("-- && || == != <= >=");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type(), TokenType::DECREMENT);
    EXPECT_EQ(tokens[1].type(), TokenType::LOGICAL_AND);
    EXPECT_EQ(tokens[2].type(), TokenType::LOGICAL_OR);
    EXPECT_EQ(tokens[3].type(), TokenType::EQUAL);
    EXPECT_EQ(tokens[4].type(), TokenType::NOT_EQUAL);
    EXPECT_EQ(tokens[5].type(), TokenType::LESS_THAN_EQUAL);
    EXPECT_EQ(tokens[6].type(), TokenType::GREATER_THAN_EQUAL);
}

TEST_F(LexerTest, SimpleFunction)
{
    std::string filepath = create_test_file("int main() { return 0; }");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 9);
    EXPECT_EQ(tokens[0].type(), TokenType::INT_KW);
    EXPECT_EQ(tokens[1].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme(), "main");
    EXPECT_EQ(tokens[2].type(), TokenType::OPEN_PAREN);
    EXPECT_EQ(tokens[3].type(), TokenType::CLOSE_PAREN);
    EXPECT_EQ(tokens[4].type(), TokenType::OPEN_BRACE);
    EXPECT_EQ(tokens[5].type(), TokenType::RETURN_KW);
    EXPECT_EQ(tokens[6].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[6].literal<int>(), 0);
    EXPECT_EQ(tokens[7].type(), TokenType::SEMICOLON);
    EXPECT_EQ(tokens[8].type(), TokenType::CLOSE_BRACE);
}

TEST_F(LexerTest, MultilineCode)
{
    std::string filepath = create_test_file(
        "int x = 5;\n"
        "int y = 10;\n"
        "int z = x + y;");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    // Check line numbers
    EXPECT_EQ(tokens[0].source_location().line_number, 1);  // int
    EXPECT_EQ(tokens[4].source_location().line_number, 1);  // ;
    EXPECT_EQ(tokens[5].source_location().line_number, 2);  // int (second line)
    EXPECT_EQ(tokens[9].source_location().line_number, 2);  // ;
    EXPECT_EQ(tokens[10].source_location().line_number, 3); // int (third line)
}

TEST_F(LexerTest, WhitespaceHandling)
{
    std::string filepath = create_test_file("  int   x  =  5  ;  ");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 5);
    EXPECT_EQ(tokens[0].type(), TokenType::INT_KW);
    EXPECT_EQ(tokens[1].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].type(), TokenType::ASSIGNMENT);
    EXPECT_EQ(tokens[3].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[4].type(), TokenType::SEMICOLON);
}

TEST_F(LexerTest, ConditionalExpression)
{
    std::string filepath = create_test_file("x > 0 ? x : -x");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].type(), TokenType::GREATER_THAN);
    EXPECT_EQ(tokens[2].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[3].type(), TokenType::QUESTION_MARK);
    EXPECT_EQ(tokens[4].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[5].type(), TokenType::COLON);
    EXPECT_EQ(tokens[6].type(), TokenType::MINUS);
    EXPECT_EQ(tokens[7].type(), TokenType::IDENTIFIER);
}

TEST_F(LexerTest, InvalidToken)
{
    std::string filepath = create_test_file("int x = 5; @");

    Lexer lexer(filepath, token_table);

    EXPECT_THROW({ lexer.tokenize(); }, LexerError);
}

TEST_F(LexerTest, ComplexExpression)
{
    std::string filepath = create_test_file("if (x >= 10 && y != 0) { z = x / y; }");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 18);
    EXPECT_EQ(tokens[0].type(), TokenType::IF_KW);
    EXPECT_EQ(tokens[1].type(), TokenType::OPEN_PAREN);
    EXPECT_EQ(tokens[2].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].type(), TokenType::GREATER_THAN_EQUAL);
    EXPECT_EQ(tokens[4].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[5].type(), TokenType::LOGICAL_AND);
    // ... and so on
}

TEST_F(LexerTest, StaticExternKeywords)
{
    std::string filepath = create_test_file("static int x; extern void func();");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type(), TokenType::STATIC_KW);
    EXPECT_EQ(tokens[4].type(), TokenType::EXTERN_KW);
}

TEST_F(LexerTest, LoopKeywords)
{
    std::string filepath = create_test_file("do { break; } while(1); for(;;) { continue; }");

    Lexer lexer(filepath, token_table);
    auto tokens = lexer.tokenize();

    // Find and check specific keywords
    bool found_do = false, found_break = false, found_continue = false;
    for (const auto& token : tokens) {
        if (token.type() == TokenType::DO_KW)
            found_do = true;
        if (token.type() == TokenType::BREAK_KW)
            found_break = true;
        if (token.type() == TokenType::CONTINUE_KW)
            found_continue = true;
    }

    EXPECT_TRUE(found_do);
    EXPECT_TRUE(found_break);
    EXPECT_TRUE(found_continue);
}
