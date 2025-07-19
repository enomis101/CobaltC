#include "parser_test/parser_test.h"

// ============== Error Handling Tests ==============

TEST_F(ParserTest, ParseError_MissingSemicolon)
{
    expect_parse_error("int main(void) { return 0 }");
}

TEST_F(ParserTest, ParseError_MissingCloseParen)
{
    expect_parse_error("int main(void) { if (x > 0 return 1; }");
}

TEST_F(ParserTest, ParseError_MissingOpenBrace)
{
    expect_parse_error("int main(void) return 0; }");
}

TEST_F(ParserTest, ParseError_InvalidExpression)
{
    expect_parse_error("int main(void) { return + ; }");
}

TEST_F(ParserTest, ParseError_InvalidForLoop)
{
    expect_parse_error("int main(void) { for (int i = 0 i < 10; i++) {} }");
}

// ============== TODOs ==============

// TODO: Type specifier errors
// TODO: Multiple identical specifiers
// TODO: Invalid combinations (`signed double`)
// TODO: Missing type specifiers

// TODO: Declarator errors  
// TODO: Invalid array sizes (negative, zero)
// TODO: Complex nested declarators
// TODO: Function pointer syntax errors

// TODO: Expression parsing errors
// TODO: Malformed cast expressions
// TODO: Invalid primary expressions
// TODO: Precedence edge cases

// TODO: Parser context tracking
// TODO: Context stack functionality
// TODO: Error reporting with context