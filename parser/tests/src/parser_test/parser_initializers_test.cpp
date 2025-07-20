#include "parser/parser_ast.h"
#include "parser_test/parser_test.h"
#include <gtest/gtest.h>

TEST_F(ParserTest, ParseCompoundInitializer)
{
    auto ast = parse_string("int y[3] = {1 , 2, 3};");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    auto init = dynamic_cast<CompoundInitializer*>(var_decl->expression.value().get());
    ASSERT_NE(init, nullptr);

    EXPECT_EQ(init->initializer_list.size(), 3);
}

TEST_F(ParserTest, ParseCompoundInitializer_2)
{
    auto ast = parse_string("int y[3] = {1 , 2, 3, };");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    auto init = dynamic_cast<CompoundInitializer*>(var_decl->expression.value().get());
    ASSERT_NE(init, nullptr);

    EXPECT_EQ(init->initializer_list.size(), 3);
}

TEST_F(ParserTest, ParseNestedCompoundInitializer)
{
    auto ast = parse_string("int y[3][4] = {{1 , 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}};");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    auto init = dynamic_cast<CompoundInitializer*>(var_decl->expression.value().get());
    ASSERT_NE(init, nullptr);

    EXPECT_EQ(init->initializer_list.size(), 3);

    for (auto& nested_init : init->initializer_list) {
        auto compound_nested_init = dynamic_cast<CompoundInitializer*>(nested_init.get());
        ASSERT_NE(compound_nested_init, nullptr);

        EXPECT_EQ(compound_nested_init->initializer_list.size(), 4);
    }
}

TEST_F(ParserTest, ParseNestedCompoundInitializer_2)
{
    auto ast = parse_string("int y[3][4] = {{1 , 2, 3, 4, }, {5, 6, 7, 8}, {9, 10, 11, 12}, };");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    auto init = dynamic_cast<CompoundInitializer*>(var_decl->expression.value().get());
    ASSERT_NE(init, nullptr);

    EXPECT_EQ(init->initializer_list.size(), 3);

    for (auto& nested_init : init->initializer_list) {
        auto compound_nested_init = dynamic_cast<CompoundInitializer*>(nested_init.get());
        ASSERT_NE(compound_nested_init, nullptr);

        EXPECT_EQ(compound_nested_init->initializer_list.size(), 4);
    }
}

// ============== Initializer Tests ==============

// TODO: Single initializers for arrays
// TODO: Compound initializers (`{1, 2, 3}`)
// TODO: Nested compound initializers for multi-dimensional arrays
// TODO: Trailing commas in initializer lists
