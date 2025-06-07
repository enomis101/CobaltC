#include "common/data/token_table.h"
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
        Lexer lexer(filepath, token_table);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        return parser.parse_program();
    }

    // Helper to check if parsing throws an exception
    void expect_parse_error(const std::string& content)
    {
        std::string filepath = create_test_file(content);
        Lexer lexer(filepath, token_table);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        EXPECT_THROW(parser.parse_program(), Parser::ParserError);
    }

    std::shared_ptr<TokenTable> token_table;
    fs::path test_dir;
};

// ============== Basic Expression Tests ==============

TEST_F(ParserTest, ParseConstantExpression)
{
    auto ast = parse_string("int main(void) { return 42; }");

    ASSERT_NE(ast, nullptr);
    ASSERT_EQ(ast->declarations.size(), 1);

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(func_decl, nullptr);
    ASSERT_TRUE(func_decl->body.has_value());

    auto block = func_decl->body.value().get();
    ASSERT_EQ(block->items.size(), 1);

    auto return_stmt = dynamic_cast<ReturnStatement*>(block->items[0].get());
    ASSERT_NE(return_stmt, nullptr);

    auto const_expr = dynamic_cast<ConstantExpression*>(return_stmt->expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 42);
}

TEST_F(ParserTest, ParseUnaryExpression)
{
    auto ast = parse_string("int main(void) { return -5; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto unary_expr = dynamic_cast<UnaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(unary_expr, nullptr);
    EXPECT_EQ(unary_expr->unary_operator, UnaryOperator::NEGATE);

    auto const_expr = dynamic_cast<ConstantExpression*>(unary_expr->expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 5);
}

TEST_F(ParserTest, ParseBinaryExpression)
{
    auto ast = parse_string("int main(void) { return 2 + 3; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::ADD);

    auto left = dynamic_cast<ConstantExpression*>(binary_expr->left_expression.get());
    auto right = dynamic_cast<ConstantExpression*>(binary_expr->right_expression.get());

    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<int>(left->value), 2);
    EXPECT_EQ(std::get<int>(right->value), 3);
}

TEST_F(ParserTest, ParsePrecedence)
{
    auto ast = parse_string("int main(void) { return 2 + 3 * 4; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto add_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    auto left = dynamic_cast<ConstantExpression*>(add_expr->left_expression.get());
    ASSERT_NE(left, nullptr);
    EXPECT_EQ(std::get<int>(left->value), 2);

    auto mult_expr = dynamic_cast<BinaryExpression*>(add_expr->right_expression.get());
    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);
}

TEST_F(ParserTest, ParseParentheses)
{
    auto ast = parse_string("int main(void) { return (2 + 3) * 4; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto mult_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);

    auto add_expr = dynamic_cast<BinaryExpression*>(mult_expr->left_expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);
}

// ============== Variable Tests ==============

TEST_F(ParserTest, ParseVariableDeclaration)
{
    auto ast = parse_string("int x = 5;");

    ASSERT_EQ(ast->declarations.size(), 1);
    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(var_decl, nullptr);
    EXPECT_EQ(var_decl->identifier.name, "x");
    EXPECT_EQ(var_decl->storage_class, StorageClass::NONE);
    EXPECT_EQ(var_decl->scope, DeclarationScope::File);
    ASSERT_TRUE(var_decl->expression.has_value());

    auto const_expr = dynamic_cast<ConstantExpression*>(var_decl->expression.value().get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 5);
}

TEST_F(ParserTest, ParseVariableExpression)
{
    auto ast = parse_string("int main(void) { int x = 5; return x; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto block = func_decl->body.value().get();
    ASSERT_EQ(block->items.size(), 2);

    auto return_stmt = dynamic_cast<ReturnStatement*>(block->items[1].get());
    auto var_expr = dynamic_cast<VariableExpression*>(return_stmt->expression.get());

    ASSERT_NE(var_expr, nullptr);
    EXPECT_EQ(var_expr->identifier.name, "x");
}

TEST_F(ParserTest, ParseAssignmentExpression)
{
    auto ast = parse_string("int main(void) { int x; x = 10; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(func_decl->body.value()->items[1].get());
    auto assign_expr = dynamic_cast<AssignmentExpression*>(expr_stmt->expression.get());

    ASSERT_NE(assign_expr, nullptr);

    auto var_expr = dynamic_cast<VariableExpression*>(assign_expr->left_expression.get());
    ASSERT_NE(var_expr, nullptr);
    EXPECT_EQ(var_expr->identifier.name, "x");

    auto const_expr = dynamic_cast<ConstantExpression*>(assign_expr->right_expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 10);
}

// ============== Function Tests ==============

TEST_F(ParserTest, ParseFunctionDeclaration)
{
    auto ast = parse_string("int main(void) { return 0; }");

    ASSERT_EQ(ast->declarations.size(), 1);
    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->name.name, "main");
    EXPECT_EQ(func_decl->params.size(), 0);
    EXPECT_EQ(func_decl->storage_class, StorageClass::NONE);
    EXPECT_EQ(func_decl->scope, DeclarationScope::File);
    ASSERT_TRUE(func_decl->body.has_value());
}

TEST_F(ParserTest, ParseFunctionWithParameters)
{
    auto ast = parse_string("int add(int a, int b) { return a + b; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->name.name, "add");
    ASSERT_EQ(func_decl->params.size(), 2);
    EXPECT_EQ(func_decl->params[0].name, "a");
    EXPECT_EQ(func_decl->params[1].name, "b");
}

TEST_F(ParserTest, ParseFunctionCall)
{
    auto ast = parse_string("int main(void) { return foo(1, 2); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto func_call = dynamic_cast<FunctionCallExpression*>(return_stmt->expression.get());

    ASSERT_NE(func_call, nullptr);
    EXPECT_EQ(func_call->name.name, "foo");
    ASSERT_EQ(func_call->arguments.size(), 2);
}

TEST_F(ParserTest, ParseFunctionPrototype)
{
    auto ast = parse_string("int foo(int x);");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->name.name, "foo");
    EXPECT_FALSE(func_decl->body.has_value());
    ASSERT_EQ(func_decl->params.size(), 1);
}

// ============== Control Flow Tests ==============

TEST_F(ParserTest, ParseIfStatement)
{
    auto ast = parse_string("int main(void) { if (x > 0) return 1; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto if_stmt = dynamic_cast<IfStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(if_stmt, nullptr);
    ASSERT_NE(if_stmt->condition, nullptr);
    ASSERT_NE(if_stmt->then_statement, nullptr);
    EXPECT_FALSE(if_stmt->else_statement.has_value());
}

TEST_F(ParserTest, ParseIfElseStatement)
{
    auto ast = parse_string("int main(void) { if (x > 0) return 1; else return 0; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto if_stmt = dynamic_cast<IfStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(if_stmt, nullptr);
    ASSERT_TRUE(if_stmt->else_statement.has_value());
    ASSERT_NE(if_stmt->else_statement.value(), nullptr);
}

TEST_F(ParserTest, ParseWhileLoop)
{
    auto ast = parse_string("int main(void) { while (x < 10) x = x + 1; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto while_stmt = dynamic_cast<WhileStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(while_stmt, nullptr);
    ASSERT_NE(while_stmt->condition, nullptr);
    ASSERT_NE(while_stmt->body, nullptr);
}

TEST_F(ParserTest, ParseDoWhileLoop)
{
    auto ast = parse_string("int main(void) { do x = x + 1; while (x < 10); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto do_while = dynamic_cast<DoWhileStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(do_while, nullptr);
    ASSERT_NE(do_while->condition, nullptr);
    ASSERT_NE(do_while->body, nullptr);
}

TEST_F(ParserTest, ParseForLoop)
{
    auto ast = parse_string("int main(void) { for (int i = 0; i < 10; i = i + 1) x = x + i; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto for_stmt = dynamic_cast<ForStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(for_stmt, nullptr);
    ASSERT_NE(for_stmt->init, nullptr);
    ASSERT_TRUE(for_stmt->condition.has_value());
    ASSERT_TRUE(for_stmt->post.has_value());
    ASSERT_NE(for_stmt->body, nullptr);

    auto for_init = dynamic_cast<ForInitDeclaration*>(for_stmt->init.get());
    ASSERT_NE(for_init, nullptr);
}

TEST_F(ParserTest, ParseBreakContinue)
{
    auto ast = parse_string("int main(void) { while (1) { if (x > 10) break; continue; } }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto while_stmt = dynamic_cast<WhileStatement*>(func_decl->body.value()->items[0].get());
    auto compound = dynamic_cast<CompoundStatement*>(while_stmt->body.get());
    auto if_stmt = dynamic_cast<IfStatement*>(compound->block->items[0].get());
    auto break_stmt = dynamic_cast<BreakStatement*>(if_stmt->then_statement.get());
    auto continue_stmt = dynamic_cast<ContinueStatement*>(compound->block->items[1].get());

    ASSERT_NE(break_stmt, nullptr);
    ASSERT_NE(continue_stmt, nullptr);
}

// ============== Storage Class Tests ==============

TEST_F(ParserTest, ParseStaticVariable)
{
    auto ast = parse_string("static int x = 5;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(var_decl, nullptr);
    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
}

TEST_F(ParserTest, ParseExternVariable)
{
    auto ast = parse_string("extern int x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(var_decl, nullptr);
    EXPECT_EQ(var_decl->storage_class, StorageClass::EXTERN);
    EXPECT_FALSE(var_decl->expression.has_value());
}

TEST_F(ParserTest, ParseStaticFunction)
{
    auto ast = parse_string("static int foo(void) { return 0; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->storage_class, StorageClass::STATIC);
}

// ============== Complex Expression Tests ==============

TEST_F(ParserTest, ParseConditionalExpression)
{
    auto ast = parse_string("int main(void) { return x > 0 ? x : -x; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto cond_expr = dynamic_cast<ConditionalExpression*>(return_stmt->expression.get());

    ASSERT_NE(cond_expr, nullptr);
    ASSERT_NE(cond_expr->condition, nullptr);
    ASSERT_NE(cond_expr->true_expression, nullptr);
    ASSERT_NE(cond_expr->false_expression, nullptr);
}

TEST_F(ParserTest, ParseLogicalOperators)
{
    auto ast = parse_string("int main(void) { return x > 0 && y < 10 || z == 0; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto or_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(or_expr, nullptr);
    EXPECT_EQ(or_expr->binary_operator, BinaryOperator::OR);

    auto and_expr = dynamic_cast<BinaryExpression*>(or_expr->left_expression.get());
    ASSERT_NE(and_expr, nullptr);
    EXPECT_EQ(and_expr->binary_operator, BinaryOperator::AND);
}

TEST_F(ParserTest, ParseComparisonOperators)
{
    auto ast = parse_string("int main(void) { return x >= 5 && y <= 10; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto and_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    auto left = dynamic_cast<BinaryExpression*>(and_expr->left_expression.get());
    auto right = dynamic_cast<BinaryExpression*>(and_expr->right_expression.get());

    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(left->binary_operator, BinaryOperator::GREATER_OR_EQUAL);
    EXPECT_EQ(right->binary_operator, BinaryOperator::LESS_OR_EQUAL);
}

// ============== Statement Tests ==============

TEST_F(ParserTest, ParseCompoundStatement)
{
    auto ast = parse_string("int main(void) { { int x = 5; return x; } }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto compound = dynamic_cast<CompoundStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(compound, nullptr);
    ASSERT_NE(compound->block, nullptr);
    ASSERT_EQ(compound->block->items.size(), 2);
}

TEST_F(ParserTest, ParseNullStatement)
{
    auto ast = parse_string("int main(void) { ; ; ; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_EQ(func_decl->body.value()->items.size(), 3);
    for (const auto& item : func_decl->body.value()->items) {
        EXPECT_NE(dynamic_cast<NullStatement*>(item.get()), nullptr);
    }
}

TEST_F(ParserTest, ParseExpressionStatement)
{
    auto ast = parse_string("int main(void) { x + 5; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(expr_stmt, nullptr);
    ASSERT_NE(expr_stmt->expression, nullptr);
}

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

// ============== Complex Program Tests ==============

TEST_F(ParserTest, ParseMultipleDeclarations)
{
    auto ast = parse_string(R"(
        int x = 5;
        int y = 10;
        int add(int a, int b) { return a + b; }
        int main(void) { return add(x, y); }
    )");

    ASSERT_EQ(ast->declarations.size(), 4);

    EXPECT_NE(dynamic_cast<VariableDeclaration*>(ast->declarations[0].get()), nullptr);
    EXPECT_NE(dynamic_cast<VariableDeclaration*>(ast->declarations[1].get()), nullptr);
    EXPECT_NE(dynamic_cast<FunctionDeclaration*>(ast->declarations[2].get()), nullptr);
    EXPECT_NE(dynamic_cast<FunctionDeclaration*>(ast->declarations[3].get()), nullptr);
}

TEST_F(ParserTest, ParseNestedLoops)
{
    auto ast = parse_string(R"(
        int main(void) {
            for (int i = 0; i < 10; i = i + 1) {
                for (int j = 0; j < 10; j = j + 1) {
                    if (i == j) continue;
                    x = x + 1;
                }
            }
        }
    )");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto outer_for = dynamic_cast<ForStatement*>(func_decl->body.value()->items[0].get());
    auto compound = dynamic_cast<CompoundStatement*>(outer_for->body.get());
    auto inner_for = dynamic_cast<ForStatement*>(compound->block->items[0].get());

    ASSERT_NE(outer_for, nullptr);
    ASSERT_NE(inner_for, nullptr);
}

TEST_F(ParserTest, ParseComplexExpression)
{
    auto ast = parse_string("int main(void) { return (a + b) * c / d % e == f && g != h || i < j; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(return_stmt->expression, nullptr);
    // The top-level operator should be OR due to precedence
    auto or_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());
    ASSERT_NE(or_expr, nullptr);
    EXPECT_EQ(or_expr->binary_operator, BinaryOperator::OR);
}

// ============== Local Scope Tests ==============

TEST_F(ParserTest, ParseLocalVariableDeclaration)
{
    auto ast = parse_string("int main(void) { int x = 5; static int y = 10; extern int z; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    ASSERT_EQ(func_decl->body.value()->items.size(), 3);

    auto var1 = dynamic_cast<VariableDeclaration*>(func_decl->body.value()->items[0].get());
    auto var2 = dynamic_cast<VariableDeclaration*>(func_decl->body.value()->items[1].get());
    auto var3 = dynamic_cast<VariableDeclaration*>(func_decl->body.value()->items[2].get());

    ASSERT_NE(var1, nullptr);
    EXPECT_EQ(var1->storage_class, StorageClass::NONE);
    EXPECT_EQ(var1->scope, DeclarationScope::Block);

    ASSERT_NE(var2, nullptr);
    EXPECT_EQ(var2->storage_class, StorageClass::STATIC);

    ASSERT_NE(var3, nullptr);
    EXPECT_EQ(var3->storage_class, StorageClass::EXTERN);
}

/* NOT SUPPORTED YET
TEST_F(ParserTest, ParseVoidFunction)
{
    auto ast = parse_string("void foo(void) { return; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());

    ASSERT_NE(return_stmt, nullptr);
    EXPECT_EQ(return_stmt->expression, nullptr);
}
*/

TEST_F(ParserTest, ParseRightAssociativity)
{
    auto ast = parse_string("int main(void) { a = b = c = 5; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(func_decl->body.value()->items[0].get());
    auto assign1 = dynamic_cast<AssignmentExpression*>(expr_stmt->expression.get());

    ASSERT_NE(assign1, nullptr);
    auto assign2 = dynamic_cast<AssignmentExpression*>(assign1->right_expression.get());
    ASSERT_NE(assign2, nullptr);
    auto assign3 = dynamic_cast<AssignmentExpression*>(assign2->right_expression.get());
    ASSERT_NE(assign3, nullptr);
}

// ============== Declaration Scope Tests ==============

TEST_F(ParserTest, ParseDeclarationScopes)
{
    auto ast = parse_string("int x; int main(void) { int y; }");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);
    EXPECT_EQ(var_decl->scope, DeclarationScope::File);

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[1].get());
    EXPECT_EQ(func_decl->scope, DeclarationScope::File);

    auto local_var = dynamic_cast<VariableDeclaration*>(func_decl->body.value()->items[0].get());
    ASSERT_NE(local_var, nullptr);
    EXPECT_EQ(local_var->scope, DeclarationScope::Block);
}
