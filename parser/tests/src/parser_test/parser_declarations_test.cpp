#include "parser_test/parser_test.h"

// ============== Variable Declaration Tests ==============

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

    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    ASSERT_NE(init, nullptr);
    auto const_expr = dynamic_cast<ConstantExpression*>(init->expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 5);
}

// ============== Function Declaration Tests ==============

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

TEST_F(ParserTest, ParseFunctionPrototype)
{
    auto ast = parse_string("int foo(int x);");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->name.name, "foo");
    EXPECT_FALSE(func_decl->body.has_value());
    ASSERT_EQ(func_decl->params.size(), 1);
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

// ============== TODOs ==============

// TODO: Parameter lists edge cases
// TODO: `void` parameter lists  
// TODO: Empty parameter lists `()`
// TODO: Complex parameter types (pointers, arrays)

// TODO: Function declarations edge cases
// TODO: Function pointers as parameters
// TODO: Complex return types
// TODO: Functions returning pointers/arrays

// TODO: Declaration combinations
// TODO: Mixed storage classes and types
// TODO: Function pointers with storage classes
// TODO: Static/extern with complex types