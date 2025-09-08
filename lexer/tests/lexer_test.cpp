#include "common//data/source_manager.h"
#include "common/data/token_table.h"
#include "common/data/warning_manager.h"
#include "lexer/lexer.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
namespace fs = std::filesystem;

// Mock WarningManager for testing
class MockWarningManager : public WarningManager {
public:
    struct Warning {
        LexerWarningType type;
        std::string message;
    };

    void raise_warning(LexerWarningType warning_type, const std::string& message) override
    {
        lexer_warnings.push_back({ warning_type, message });
    }

    void raise_warning(ParserWarningType warning_type, const std::string& message) override
    {
        // Not used in lexer tests
    }

    void clear_warnings()
    {
        lexer_warnings.clear();
    }

    bool has_warnings() const
    {
        return !lexer_warnings.empty();
    }

    size_t warning_count() const
    {
        return lexer_warnings.size();
    }

    const std::vector<Warning>& get_lexer_warnings() const
    {
        return lexer_warnings;
    }

private:
    std::vector<Warning> lexer_warnings;
};

class LexerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // Create shared objects for all tests
        token_table = std::make_shared<TokenTable>();
        source_manager = std::make_shared<SourceManager>();
        warning_manager = std::make_shared<MockWarningManager>();

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

    // Helper function to create a Lexer with the new constructor
    Lexer create_lexer(const std::string& filepath)
    {
        LexerContext context {
            .file_path = filepath,
            .token_table = token_table,
            .source_manager = source_manager,
            .warning_manager = warning_manager
        };
        return Lexer(context);
    }

    // Helper function to get mock warning manager
    MockWarningManager* get_mock_warning_manager()
    {
        return static_cast<MockWarningManager*>(warning_manager.get());
    }

    std::shared_ptr<TokenTable> token_table;
    std::shared_ptr<SourceManager> source_manager;
    std::shared_ptr<WarningManager> warning_manager;
    fs::path test_dir;
};

TEST_F(LexerTest, FileNotFound)
{
    std::string nonexistent_file = (test_dir / "nonexistent.i").string();

    EXPECT_THROW({ auto lexer = create_lexer(nonexistent_file); }, LexerError);
}

TEST_F(LexerTest, InvalidFileExtension)
{
    std::string wrong_extension_file = create_test_file("int main() {}", "test.c");

    EXPECT_THROW({ auto lexer = create_lexer(wrong_extension_file); }, LexerError);
}

TEST_F(LexerTest, EmptyFile)
{
    std::string empty_file = create_test_file("");

    EXPECT_THROW({ auto lexer = create_lexer(empty_file); }, LexerError);
}

TEST_F(LexerTest, SimpleIntegerConstant)
{
    std::string filepath = create_test_file("42 ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "42");
    EXPECT_EQ(tokens[0].literal<int>(), 42);
    EXPECT_EQ(tokens[0].source_location().line_number, 1);
}

TEST_F(LexerTest, LongConstants)
{
    std::string filepath = create_test_file("123L 456l ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);

    // Test uppercase L suffix
    EXPECT_EQ(tokens[0].type(), TokenType::LONG_CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "123L");
    EXPECT_EQ(tokens[0].literal<long>(), 123L);
    EXPECT_EQ(tokens[0].source_location().line_number, 1);

    // Test lowercase l suffix
    EXPECT_EQ(tokens[1].type(), TokenType::LONG_CONSTANT);
    EXPECT_EQ(tokens[1].lexeme(), "456l");
    EXPECT_EQ(tokens[1].literal<long>(), 456L);
    EXPECT_EQ(tokens[1].source_location().line_number, 1);
}

TEST_F(LexerTest, UnsignedConstants)
{
    std::string filepath = create_test_file("123U 456u ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);

    // Test uppercase U suffix
    EXPECT_EQ(tokens[0].type(), TokenType::UNSIGNED_CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "123U");
    EXPECT_EQ(tokens[0].literal<unsigned int>(), 123U);
    EXPECT_EQ(tokens[0].source_location().line_number, 1);

    // Test lowercase u suffix
    EXPECT_EQ(tokens[1].type(), TokenType::UNSIGNED_CONSTANT);
    EXPECT_EQ(tokens[1].lexeme(), "456u");
    EXPECT_EQ(tokens[1].literal<unsigned int>(), 456U);
    EXPECT_EQ(tokens[1].source_location().line_number, 1);
}

TEST_F(LexerTest, UnsignedLongConstants)
{
    std::string filepath = create_test_file("123UL 456ul 789LU 101lu ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    // Test UL suffix
    EXPECT_EQ(tokens[0].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "123UL");
    EXPECT_EQ(tokens[0].literal<unsigned long>(), 123UL);
    EXPECT_EQ(tokens[0].source_location().line_number, 1);

    // Test ul suffix
    EXPECT_EQ(tokens[1].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[1].lexeme(), "456ul");
    EXPECT_EQ(tokens[1].literal<unsigned long>(), 456UL);
    EXPECT_EQ(tokens[1].source_location().line_number, 1);

    // Test LU suffix
    EXPECT_EQ(tokens[2].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[2].lexeme(), "789LU");
    EXPECT_EQ(tokens[2].literal<unsigned long>(), 789UL);
    EXPECT_EQ(tokens[2].source_location().line_number, 1);

    // Test lu suffix
    EXPECT_EQ(tokens[3].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[3].lexeme(), "101lu");
    EXPECT_EQ(tokens[3].literal<unsigned long>(), 101UL);
    EXPECT_EQ(tokens[3].source_location().line_number, 1);
}

TEST_F(LexerTest, MixedConstantTypes)
{
    std::string filepath = create_test_file("int x = 42; long y = 100L; unsigned int z = 50U; unsigned long w = 200UL;");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    // Find and verify the constant tokens
    std::vector<std::pair<TokenType, std::string>> expected_constants = {
        { TokenType::CONSTANT, "42" },
        { TokenType::LONG_CONSTANT, "100L" },
        { TokenType::UNSIGNED_CONSTANT, "50U" },
        { TokenType::UNSIGNED_LONG_CONSTANT, "200UL" }
    };

    int constant_index = 0;
    for (const auto& token : tokens) {
        if (token.type() == TokenType::CONSTANT || token.type() == TokenType::LONG_CONSTANT || token.type() == TokenType::UNSIGNED_CONSTANT || token.type() == TokenType::UNSIGNED_LONG_CONSTANT) {

            ASSERT_LT(constant_index, expected_constants.size());
            EXPECT_EQ(token.type(), expected_constants[constant_index].first);
            EXPECT_EQ(token.lexeme(), expected_constants[constant_index].second);
            constant_index++;
        }
    }

    EXPECT_EQ(constant_index, expected_constants.size());
}

TEST_F(LexerTest, LargeConstants)
{
    std::string filepath = create_test_file("2147483647L 4294967295U 18446744073709551615UL ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 3);

    // Test large long constant
    EXPECT_EQ(tokens[0].type(), TokenType::LONG_CONSTANT);
    EXPECT_EQ(tokens[0].lexeme(), "2147483647L");
    EXPECT_EQ(tokens[0].literal<long>(), 2147483647L);

    // Test large unsigned constant
    EXPECT_EQ(tokens[1].type(), TokenType::UNSIGNED_CONSTANT);
    EXPECT_EQ(tokens[1].lexeme(), "4294967295U");
    EXPECT_EQ(tokens[1].literal<unsigned int>(), 4294967295U);

    // Test large unsigned long constant
    EXPECT_EQ(tokens[2].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[2].lexeme(), "18446744073709551615UL");
    EXPECT_EQ(tokens[2].literal<unsigned long>(), 18446744073709551615UL);
}

// New test cases for warning functionality
TEST_F(LexerTest, WarningForIntegerOverflow)
{
    // Create a constant that exceeds INT_MAX to trigger automatic promotion to long
    std::string filepath = create_test_file("2147483648 "); // INT_MAX + 1

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);

    // Should be promoted to long automatically
    EXPECT_EQ(tokens[0].type(), TokenType::LONG_CONSTANT);
    EXPECT_EQ(tokens[0].literal<long>(), 2147483648L);

    // Check that a warning was raised
    auto mock_manager = get_mock_warning_manager();
    EXPECT_TRUE(mock_manager->has_warnings());
    EXPECT_EQ(mock_manager->warning_count(), 1);

    const auto& warnings = mock_manager->get_lexer_warnings();
    EXPECT_EQ(warnings[0].type, LexerWarningType::CAST);
    EXPECT_TRUE(warnings[0].message.find("automatically promoting to long") != std::string::npos);
}

TEST_F(LexerTest, WarningForUnsignedIntegerOverflow)
{
    // Create an unsigned constant that exceeds UINT_MAX to trigger automatic promotion
    std::string filepath = create_test_file("4294967296U "); // UINT_MAX + 1

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);

    // Should be promoted to unsigned long automatically
    EXPECT_EQ(tokens[0].type(), TokenType::UNSIGNED_LONG_CONSTANT);
    EXPECT_EQ(tokens[0].literal<unsigned long>(), 4294967296UL);

    // Check that a warning was raised
    auto mock_manager = get_mock_warning_manager();
    EXPECT_TRUE(mock_manager->has_warnings());
    EXPECT_EQ(mock_manager->warning_count(), 1);

    const auto& warnings = mock_manager->get_lexer_warnings();
    EXPECT_EQ(warnings[0].type, LexerWarningType::CAST);
    EXPECT_TRUE(warnings[0].message.find("automatically promoting to unsigned long") != std::string::npos);
}

TEST_F(LexerTest, NoWarningForValidConstants)
{
    std::string filepath = create_test_file("42 100L 50U 200UL ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    // Check that no warnings were raised
    auto mock_manager = get_mock_warning_manager();
    EXPECT_FALSE(mock_manager->has_warnings());
    EXPECT_EQ(mock_manager->warning_count(), 0);
}

TEST_F(LexerTest, MultipleWarnings)
{
    // Create multiple constants that will trigger warnings
    std::string filepath = create_test_file("2147483648 4294967296U ");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 2);

    // Check that multiple warnings were raised
    auto mock_manager = get_mock_warning_manager();
    EXPECT_TRUE(mock_manager->has_warnings());
    EXPECT_EQ(mock_manager->warning_count(), 2);

    const auto& warnings = mock_manager->get_lexer_warnings();
    EXPECT_EQ(warnings[0].type, LexerWarningType::CAST);
    EXPECT_EQ(warnings[1].type, LexerWarningType::CAST);
}

// Add these tests to your existing LexerTest class

// Character Literal Tests
TEST_F(LexerTest, SimpleCharacterLiterals)
{
    std::string filepath = create_test_file("'a' 'Z' '5' ' '");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    // Test 'a' -> ASCII 97
    EXPECT_EQ(tokens[0].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[0].lexeme(), "'a'");
    EXPECT_EQ(tokens[0].literal<int>(), 97);

    // Test 'Z' -> ASCII 90
    EXPECT_EQ(tokens[1].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[1].lexeme(), "'Z'");
    EXPECT_EQ(tokens[1].literal<int>(), 90);

    // Test '5' -> ASCII 53
    EXPECT_EQ(tokens[2].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[2].lexeme(), "'5'");
    EXPECT_EQ(tokens[2].literal<int>(), 53);

    // Test ' ' (space) -> ASCII 32
    EXPECT_EQ(tokens[3].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[3].lexeme(), "' '");
    EXPECT_EQ(tokens[3].literal<int>(), 32);
}

TEST_F(LexerTest, CharacterLiteralEscapeSequences)
{
    std::string filepath = create_test_file("'\\'' '\\\"' '\\?' '\\\\' '\\a' '\\b' '\\f' '\\n' '\\r' '\\t' '\\v'");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 11);

    // Test all valid escape sequences from Table 16-1
    EXPECT_EQ(tokens[0].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[0].lexeme(), "'\\''");
    EXPECT_EQ(tokens[0].literal<int>(), 39); // Single quote

    EXPECT_EQ(tokens[1].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[1].lexeme(), "'\\\"'");
    EXPECT_EQ(tokens[1].literal<int>(), 34); // Double quote

    EXPECT_EQ(tokens[2].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[2].lexeme(), "'\\?'");
    EXPECT_EQ(tokens[2].literal<int>(), 63); // Question mark

    EXPECT_EQ(tokens[3].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[3].lexeme(), "'\\\\'");
    EXPECT_EQ(tokens[3].literal<int>(), 92); // Backslash

    EXPECT_EQ(tokens[4].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[4].lexeme(), "'\\a'");
    EXPECT_EQ(tokens[4].literal<int>(), 7); // Audible alert

    EXPECT_EQ(tokens[5].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[5].lexeme(), "'\\b'");
    EXPECT_EQ(tokens[5].literal<int>(), 8); // Backspace

    EXPECT_EQ(tokens[6].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[6].lexeme(), "'\\f'");
    EXPECT_EQ(tokens[6].literal<int>(), 12); // Form feed

    EXPECT_EQ(tokens[7].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[7].lexeme(), "'\\n'");
    EXPECT_EQ(tokens[7].literal<int>(), 10); // New line

    EXPECT_EQ(tokens[8].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[8].lexeme(), "'\\r'");
    EXPECT_EQ(tokens[8].literal<int>(), 13); // Carriage return

    EXPECT_EQ(tokens[9].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[9].lexeme(), "'\\t'");
    EXPECT_EQ(tokens[9].literal<int>(), 9); // Horizontal tab

    EXPECT_EQ(tokens[10].type(), TokenType::CHAR_LITERAL);
    EXPECT_EQ(tokens[10].lexeme(), "'\\v'");
    EXPECT_EQ(tokens[10].literal<int>(), 11); // Vertical tab
}

TEST_F(LexerTest, InvalidCharacterLiterals)
{
    // Test unterminated character literal
    std::string filepath1 = create_test_file("'a");
    auto lexer1 = create_lexer(filepath1);
    EXPECT_THROW({ lexer1.tokenize(); }, LexerError);

    // Test empty character literal
    std::string filepath2 = create_test_file("''");
    auto lexer2 = create_lexer(filepath2);
    EXPECT_THROW({ lexer2.tokenize(); }, LexerError);

    // Test invalid escape sequence
    std::string filepath3 = create_test_file("'\\x'");
    auto lexer3 = create_lexer(filepath3);
    EXPECT_THROW({ lexer3.tokenize(); }, LexerError);

    // Test character literal with newline (should be escaped)
    std::string filepath4 = create_test_file("'\n'");
    auto lexer4 = create_lexer(filepath4);
    EXPECT_THROW({ lexer4.tokenize(); }, LexerError);

    // Test multiple characters (not supported in this implementation)
    std::string filepath5 = create_test_file("'ab'");
    auto lexer5 = create_lexer(filepath5);
    EXPECT_THROW({ lexer5.tokenize(); }, LexerError);
}

// String Literal Tests
TEST_F(LexerTest, SimpleStringLiterals)
{
    std::string filepath = create_test_file("\"hello\" \"world\" \"123\" \"\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme(), "\"hello\"");
    EXPECT_EQ(tokens[0].literal<std::string>(), "hello");

    EXPECT_EQ(tokens[1].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[1].lexeme(), "\"world\"");
    EXPECT_EQ(tokens[1].literal<std::string>(), "world");

    EXPECT_EQ(tokens[2].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme(), "\"123\"");
    EXPECT_EQ(tokens[2].literal<std::string>(), "123");

    // Test empty string
    EXPECT_EQ(tokens[3].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[3].lexeme(), "\"\"");
    EXPECT_EQ(tokens[3].literal<std::string>(), "");
}

TEST_F(LexerTest, StringLiteralsWithEscapeSequences)
{
    std::string filepath = create_test_file("\"Hello\\nWorld\" \"Tab\\there\" \"Quote: \\\"text\\\"\" \"Backslash: \\\\\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    EXPECT_EQ(tokens[0].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].lexeme(), "\"Hello\\nWorld\"");
    EXPECT_EQ(tokens[0].literal<std::string>(), "Hello\nWorld");

    EXPECT_EQ(tokens[1].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[1].lexeme(), "\"Tab\\there\"");
    EXPECT_EQ(tokens[1].literal<std::string>(), "Tab\there");

    EXPECT_EQ(tokens[2].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[2].lexeme(), "\"Quote: \\\"text\\\"\"");
    EXPECT_EQ(tokens[2].literal<std::string>(), "Quote: \"text\"");

    EXPECT_EQ(tokens[3].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[3].lexeme(), "\"Backslash: \\\\\"");
    EXPECT_EQ(tokens[3].literal<std::string>(), "Backslash: \\");
}

TEST_F(LexerTest, StringLiteralsWithAllEscapeSequences)
{
    std::string filepath = create_test_file("\"\\' \\\" \\? \\\\ \\a \\b \\f \\n \\r \\t \\v\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);

    EXPECT_EQ(tokens[0].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].literal<std::string>(), "' \" ? \\ \a \b \f \n \r \t \v");
}

TEST_F(LexerTest, StringLiteralWithSingleQuotes)
{
    // Single quotes don't need to be escaped in string literals
    std::string filepath = create_test_file("\"Don't worry\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].literal<std::string>(), "Don't worry");
}

TEST_F(LexerTest, InvalidStringLiterals)
{
    // Test unterminated string literal
    std::string filepath1 = create_test_file("\"hello");
    auto lexer1 = create_lexer(filepath1);
    EXPECT_THROW({ lexer1.tokenize(); }, LexerError);

    // Test string with newline (should be escaped)
    std::string filepath2 = create_test_file("\"hello\nworld\"");
    auto lexer2 = create_lexer(filepath2);
    EXPECT_THROW({ lexer2.tokenize(); }, LexerError);

    // Test string with invalid escape sequence
    std::string filepath3 = create_test_file("\"hello\\x\"");
    auto lexer3 = create_lexer(filepath3);
    EXPECT_THROW({ lexer3.tokenize(); }, LexerError);

    // Test string with unescaped backslash at end
    std::string filepath4 = create_test_file("\"hello\\\"");
    auto lexer4 = create_lexer(filepath4);
    EXPECT_THROW({ lexer4.tokenize(); }, LexerError);
}

TEST_F(LexerTest, CharAndStringInContext)
{
    std::string filepath = create_test_file("char c = 'a'; char* str = \"hello\";");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    // Find the character and string literals
    bool found_char = false, found_string = false;
    for (const auto& token : tokens) {
        if (token.type() == TokenType::CHAR_LITERAL) {
            EXPECT_EQ(token.lexeme(), "'a'");
            EXPECT_EQ(token.literal<int>(), 97);
            found_char = true;
        }
        if (token.type() == TokenType::STRING_LITERAL) {
            EXPECT_EQ(token.lexeme(), "\"hello\"");
            EXPECT_EQ(token.literal<std::string>(), "hello");
            found_string = true;
        }
    }

    EXPECT_TRUE(found_char);
    EXPECT_TRUE(found_string);
}

TEST_F(LexerTest, ComplexStringWithMixedEscapes)
{
    std::string filepath = create_test_file("\"Line 1\\nLine 2\\tTabbed\\\"Quoted\\\" \\\\Path\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::STRING_LITERAL);
    EXPECT_EQ(tokens[0].literal<std::string>(), "Line 1\nLine 2\tTabbed\"Quoted\" \\Path");
}

TEST_F(LexerTest, EdgeCaseEscapeSequences)
{
    // Test question mark - can be escaped but doesn't need to be
    std::string filepath = create_test_file("'?' '\\?' \"?\" \"\\?\"");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 4);

    // Both '?' and '\?' should give the same result
    EXPECT_EQ(tokens[0].literal<int>(), 63);
    EXPECT_EQ(tokens[1].literal<int>(), 63);

    // Both "?" and "\?" should give the same result
    EXPECT_EQ(tokens[2].literal<std::string>(), "?");
    EXPECT_EQ(tokens[3].literal<std::string>(), "?");
}

TEST_F(LexerTest, SimpleIdentifier)
{
    std::string filepath = create_test_file("myVariable");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].lexeme(), "myVariable");
    EXPECT_EQ(tokens[0].source_location().line_number, 1);
}

TEST_F(LexerTest, Keywords)
{
    std::string filepath = create_test_file("int return void if else while for");

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);
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

    auto lexer = create_lexer(filepath);

    EXPECT_THROW({ lexer.tokenize(); }, LexerError);
}

TEST_F(LexerTest, ComplexExpression)
{
    std::string filepath = create_test_file("if (x >= 10 && y != 0) { z = x / y; }");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    ASSERT_EQ(tokens.size(), 18);
    EXPECT_EQ(tokens[0].type(), TokenType::IF_KW);
    EXPECT_EQ(tokens[1].type(), TokenType::OPEN_PAREN);
    EXPECT_EQ(tokens[2].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[2].lexeme(), "x");
    EXPECT_EQ(tokens[3].type(), TokenType::GREATER_THAN_EQUAL);
    EXPECT_EQ(tokens[4].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[4].literal<int>(), 10);
    EXPECT_EQ(tokens[5].type(), TokenType::LOGICAL_AND);
    EXPECT_EQ(tokens[6].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[6].lexeme(), "y");
    EXPECT_EQ(tokens[7].type(), TokenType::NOT_EQUAL);
    EXPECT_EQ(tokens[8].type(), TokenType::CONSTANT);
    EXPECT_EQ(tokens[8].literal<int>(), 0);
    EXPECT_EQ(tokens[9].type(), TokenType::CLOSE_PAREN);
    EXPECT_EQ(tokens[10].type(), TokenType::OPEN_BRACE);
    EXPECT_EQ(tokens[11].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[11].lexeme(), "z");
    EXPECT_EQ(tokens[12].type(), TokenType::ASSIGNMENT);
    EXPECT_EQ(tokens[13].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[13].lexeme(), "x");
    EXPECT_EQ(tokens[14].type(), TokenType::FORWARD_SLASH);
    EXPECT_EQ(tokens[15].type(), TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[15].lexeme(), "y");
    EXPECT_EQ(tokens[16].type(), TokenType::SEMICOLON);
    EXPECT_EQ(tokens[17].type(), TokenType::CLOSE_BRACE);
}

TEST_F(LexerTest, StaticExternKeywords)
{
    std::string filepath = create_test_file("static int x; extern void func();");

    auto lexer = create_lexer(filepath);
    auto tokens = lexer.tokenize();

    EXPECT_EQ(tokens[0].type(), TokenType::STATIC_KW);
    EXPECT_EQ(tokens[4].type(), TokenType::EXTERN_KW);
}

TEST_F(LexerTest, LoopKeywords)
{
    std::string filepath = create_test_file("do { break; } while(1); for(;;) { continue; }");

    auto lexer = create_lexer(filepath);
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
