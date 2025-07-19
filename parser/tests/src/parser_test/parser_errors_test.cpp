#include "parser_test/parser_test.h"

// ============== Error Handling Tests ==============

TEST_F(ParserTest, ParseError_MissingSemicolon)
{
    expect_parse_error<Parser::ParserError>("int main(void) { return 0 }");
}

TEST_F(ParserTest, ParseError_MissingCloseParen)
{
    expect_parse_error<Parser::ParserError>("int main(void) { if (x > 0 return 1; }");
}

TEST_F(ParserTest, ParseError_MissingOpenBrace)
{
    expect_parse_error<Parser::ParserError>("int main(void) return 0; }");
}

TEST_F(ParserTest, ParseError_InvalidExpression)
{
    expect_parse_error<Parser::ParserError>("int main(void) { return + ; }");
}

TEST_F(ParserTest, ParseError_InvalidForLoop)
{
    expect_parse_error<Parser::ParserError>("int main(void) { for (int i = 0 i < 10; i++) {} }");
}

// Type specifier errors
TEST_F(ParserTest, ParseError_MultipleTypeSpecifiers)
{
    expect_parse_error<Parser::ParserError>("int main(void) { int int x = 0; }");
}

TEST_F(ParserTest, ParseError_InvalidTypeSpecifiers)
{
    expect_parse_error<Parser::ParserError>("int main(void) { int void x = 0; }");
}

TEST_F(ParserTest, ParseError_EmptyTypeSpecifiers)
{
    expect_parse_error<Parser::ParserError>("int main(void) { static x = 0; }");
}

TEST_F(ParserTest, ParseError_InvalidCombinationTypeSpecifiers)
{
    expect_parse_error<Parser::ParserError>("int main(void) { int signed unsigned x = 0; }");
}

TEST_F(ParserTest, ParseError_InvalidCombinationTypeSpecifiers_Double)
{
    expect_parse_error<Parser::ParserError>("int main(void) { signed double x = 0; }");
}

// Declarator errors

TEST_F(ParserTest, ParseError_InvalidArraySize_Zero)
{
    expect_parse_error<Parser::ParserError>("int main(void) { int x[0] = 0; }");
}
TEST_F(ParserTest, ParseError_InvalidArraySize_Negative)
{
    expect_parse_error<Parser::ParserError>("int main(void) { int x[-1] = 0; }");
}

// ============== TODOs ==============
// TODO: Declarator errors
// TODO: Complex nested declarators

// TODO: Expression parsing errors
// TODO: Malformed cast expressions
// TODO: Invalid primary expressions
// TODO: Precedence edge cases

// TODO: Parser context tracking
// TODO: Context stack functionality
// TODO: Error reporting with context
