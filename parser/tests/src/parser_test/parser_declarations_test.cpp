#include "common/data/type.h"
#include "common/error/internal_compiler_error.h"
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

TEST_F(ParserTest, ParseFunctionDeclarationWithVoidParameters)
{
    auto ast = parse_string("int add(void) { return 10; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);
    EXPECT_EQ(func_decl->name.name, "add");
    ASSERT_EQ(func_decl->params.size(), 0);
}

TEST_F(ParserTest, ParseFunctionDeclarationWithEmptyParameters)
{
    // COBALTC_SPECIFIC
    // cant declare empty parametr list

    EXPECT_THROW(parse_string("int add() { return 10; }"), Parser::ParserError);
}

TEST_F(ParserTest, ParseFunctionDeclarationWithComplexParameters)
{
    auto ast = parse_string("int add(int a, int* ptr, int arr[3], int** ptr_to_ptr, int matrix[5][10]) { return a + b; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);

    auto func_type = dynamic_cast<FunctionType*>(func_decl->type.get());
    ASSERT_NE(func_type, nullptr);

    EXPECT_EQ(func_decl->name.name, "add");
    ASSERT_EQ(func_decl->params.size(), 5);
    EXPECT_EQ(func_decl->params[0].name, "a");
    EXPECT_EQ(func_decl->params[1].name, "ptr");
    EXPECT_EQ(func_decl->params[2].name, "arr");
    EXPECT_EQ(func_decl->params[3].name, "ptr_to_ptr");
    EXPECT_EQ(func_decl->params[4].name, "matrix");
    EXPECT_TRUE(func_decl->body.has_value());

    EXPECT_TRUE(is_type<IntType>(*func_type->return_type));
    EXPECT_TRUE(is_type<IntType>(*func_type->parameters_type[0]));
    EXPECT_TRUE(is_type<PointerType>(*func_type->parameters_type[1]));
    EXPECT_TRUE(is_type<ArrayType>(*func_type->parameters_type[2]));
    auto ptr_to_ptr_type = dynamic_cast<PointerType*>(func_type->parameters_type[3].get());
    EXPECT_TRUE(ptr_to_ptr_type && is_type<PointerType>(*ptr_to_ptr_type->referenced_type));
    auto matrix_type = dynamic_cast<ArrayType*>(func_type->parameters_type[4].get());
    EXPECT_TRUE(matrix_type && is_type<ArrayType>(*matrix_type->element_type));
}

TEST_F(ParserTest, ParseFunctionDeclarationWithFunctionPointerParameter)
{
    // COBALTC_SPECIFIC
    // do not support function pointers as parameters

    EXPECT_THROW(parse_string("int add(int (*func)(int, int)) { return 10; }"), Parser::ParserError);
}

TEST_F(ParserTest, ParseFunctionDeclarationPointerReturnType)
{
    auto ast = parse_string("int* add(void) { return 0; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());

    ASSERT_NE(func_decl, nullptr);

    auto func_type = dynamic_cast<FunctionType*>(func_decl->type.get());
    ASSERT_NE(func_type, nullptr);

    EXPECT_EQ(func_decl->name.name, "add");
    ASSERT_EQ(func_decl->params.size(), 0);
    EXPECT_TRUE(func_decl->body.has_value());

    EXPECT_TRUE(is_type<PointerType>(*func_type->return_type));
}

TEST_F(ParserTest, ParseFunctionDeclarationArrayReturnType)
{
    // cant return array type
    EXPECT_THROW(parse_string("int[3] add(void) { return 0; }"), Parser::ParserError);
}

// Add these to your existing file:

TEST_F(ParserTest, ParseFunctionDeclarationPointerToPointerReturnType)
{
    auto ast = parse_string("int** add(void) { return 0; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(func_decl, nullptr);

    auto func_type = dynamic_cast<FunctionType*>(func_decl->type.get());
    ASSERT_NE(func_type, nullptr);

    EXPECT_EQ(func_decl->name.name, "add");
    ASSERT_EQ(func_decl->params.size(), 0);
    EXPECT_TRUE(func_decl->body.has_value());

    // Should be pointer to pointer to int
    auto ptr_type = dynamic_cast<PointerType*>(func_type->return_type.get());
    ASSERT_NE(ptr_type, nullptr);
    EXPECT_TRUE(is_type<PointerType>(*ptr_type->referenced_type));
}

// ============== Declaration Combinations Tests ==============

TEST_F(ParserTest, ParseStaticPointerVariable)
{
    auto ast = parse_string("static int* ptr;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
    EXPECT_TRUE(is_type<PointerType>(*var_decl->type));
    EXPECT_FALSE(var_decl->expression.has_value());
}

TEST_F(ParserTest, ParseExternPointerVariable)
{
    auto ast = parse_string("extern int* global_ptr;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::EXTERN);
    EXPECT_TRUE(is_type<PointerType>(*var_decl->type));
    EXPECT_FALSE(var_decl->expression.has_value());
}

TEST_F(ParserTest, ParseStaticArrayVariable)
{
    auto ast = parse_string("static int arr[10];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
    EXPECT_TRUE(is_type<ArrayType>(*var_decl->type));

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 10);
}

TEST_F(ParserTest, ParseStaticArrayVariable_UnsignedSize)
{
    auto ast = parse_string("static int arr[10u];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
    EXPECT_TRUE(is_type<ArrayType>(*var_decl->type));

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 10);
}

TEST_F(ParserTest, ParseStaticArrayVariable_UnsignedLongSize)
{
    auto ast = parse_string("static int arr[10ul];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
    EXPECT_TRUE(is_type<ArrayType>(*var_decl->type));

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 10);
}

TEST_F(ParserTest, ParseStaticArrayVariable_LongSize)
{
    auto ast = parse_string("static int arr[10l];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);
    EXPECT_TRUE(is_type<ArrayType>(*var_decl->type));

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 10);
}

TEST_F(ParserTest, ParseExternArrayVariable)
{
    auto ast = parse_string("extern int global_arr[100];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::EXTERN);
    EXPECT_TRUE(is_type<ArrayType>(*var_decl->type));

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 100);
}

TEST_F(ParserTest, ParseStaticPointerToPointer)
{
    auto ast = parse_string("static int** ptr_to_ptr;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);

    auto ptr_type = dynamic_cast<PointerType*>(var_decl->type.get());
    ASSERT_NE(ptr_type, nullptr);
    EXPECT_TRUE(is_type<PointerType>(*ptr_type->referenced_type));
}

TEST_F(ParserTest, ParseExternMultiDimensionalArray)
{
    auto ast = parse_string("extern int matrix[5][10];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::EXTERN);

    auto outer_array = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(outer_array, nullptr);
    EXPECT_EQ(outer_array->array_size, 5);

    auto inner_array = dynamic_cast<ArrayType*>(outer_array->element_type.get());
    ASSERT_NE(inner_array, nullptr);
    EXPECT_EQ(inner_array->array_size, 10);
}

TEST_F(ParserTest, ParseStaticArrayOfPointers)
{
    auto ast = parse_string("static int* ptr_array[5];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    EXPECT_EQ(var_decl->storage_class, StorageClass::STATIC);

    auto array_type = dynamic_cast<ArrayType*>(var_decl->type.get());
    ASSERT_NE(array_type, nullptr);
    EXPECT_EQ(array_type->array_size, 5);
    EXPECT_TRUE(is_type<PointerType>(*array_type->element_type));
}

// Function pointers - should fail since function pointers aren't supported
TEST_F(ParserTest, ParseFunctionPointer_ShouldFail)
{
    // COBALTC_SPECIFIC
    // function pointers not supported
    EXPECT_THROW(parse_string("int (*global_func_ptr)(void);"), UnsupportedFeatureError);
}

// ============== Multiple Storage Classes Error Test ==============

TEST_F(ParserTest, ParseMultipleStorageClasses_ShouldFail)
{
    // Should fail - can't have both static and extern
    EXPECT_THROW(parse_string("static extern int x;"), Parser::ParserError);
}
