#include "parser_test/parser_test.h"

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

// ============== TODOs ==============

// TODO: Statement parsing edge cases
// TODO: Empty for loop components
// TODO: Nested conditional statements - partially covered
// TODO: Complex loop body statements
