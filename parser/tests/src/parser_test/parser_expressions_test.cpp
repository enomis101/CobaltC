#include "common/data/type.h"
#include "parser/parser_ast.h"
#include "parser/parser_printer.h"
#include "parser_test/parser_test.h"

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

TEST_F(ParserTest, ParseUnaryExpression_Complement)
{
    auto ast = parse_string("int main(void) { return ~5; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto unary_expr = dynamic_cast<UnaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(unary_expr, nullptr);
    EXPECT_EQ(unary_expr->unary_operator, UnaryOperator::COMPLEMENT);

    auto const_expr = dynamic_cast<ConstantExpression*>(unary_expr->expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 5);
}

TEST_F(ParserTest, ParseUnaryExpression_Negate)
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


TEST_F(ParserTest, ParseUnaryExpression_Not)
{
    auto ast = parse_string("int main(void) { return !5; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto unary_expr = dynamic_cast<UnaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(unary_expr, nullptr);
    EXPECT_EQ(unary_expr->unary_operator, UnaryOperator::NOT);

    auto const_expr = dynamic_cast<ConstantExpression*>(unary_expr->expression.get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 5);
}

// TODO: Binary operators edge cases
// TODO: Modulo operator (`%`)
// TODO: All combinations and precedence

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

// ============== Assignment Expression Tests ==============

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

TEST_F(ParserTest, ParseComparisonOperators2)
{
    auto ast = parse_string("int main(void) { return x == 5 && y != 10; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto and_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    auto left = dynamic_cast<BinaryExpression*>(and_expr->left_expression.get());
    auto right = dynamic_cast<BinaryExpression*>(and_expr->right_expression.get());

    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(left->binary_operator, BinaryOperator::EQUAL);
    EXPECT_EQ(right->binary_operator, BinaryOperator::NOT_EQUAL);
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

// ============== Cast expressions Tests ==============

TEST_F(ParserTest, ParseSimpleCastExpression)
{
    auto ast = parse_string("long y = (long) x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    EXPECT_NE(var_decl, nullptr);

    EXPECT_TRUE(var_decl->expression.has_value());
    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    EXPECT_TRUE(init);

    auto cast_expr = dynamic_cast<CastExpression*>(init->expression.get());

    EXPECT_TRUE(cast_expr);
    EXPECT_TRUE(is_type<LongType>(*cast_expr->target_type));
}

TEST_F(ParserTest, ParsePointerCastExpression)
{
    auto ast = parse_string("long* y = (long*) x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    EXPECT_NE(var_decl, nullptr);

    EXPECT_TRUE(var_decl->expression.has_value());
    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    EXPECT_TRUE(init);

    auto expr = dynamic_cast<CastExpression*>(init->expression.get());

    EXPECT_TRUE(expr);
    EXPECT_TRUE(is_type<PointerType>(*expr->target_type));
}

// ============== Address-of operator Tests ==============

TEST_F(ParserTest, ParseAddressOfExpression)
{
    auto ast = parse_string("long* y = &x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    EXPECT_NE(var_decl, nullptr);

    EXPECT_TRUE(var_decl->expression.has_value());
    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    EXPECT_TRUE(init);

    auto expr = dynamic_cast<AddressOfExpression*>(init->expression.get());

    EXPECT_TRUE(expr);
}

TEST_F(ParserTest, ParseDereferenceExpression)
{
    auto ast = parse_string("long* y = *x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    EXPECT_NE(var_decl, nullptr);

    EXPECT_TRUE(var_decl->expression.has_value());
    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    EXPECT_TRUE(init);

    auto expr = dynamic_cast<DereferenceExpression*>(init->expression.get());

    EXPECT_TRUE(expr);
}

TEST_F(ParserTest, ParseSubscriptExpression)
{
    auto ast = parse_string("long y = x[0];");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    EXPECT_NE(var_decl, nullptr);

    EXPECT_TRUE(var_decl->expression.has_value());
    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    EXPECT_TRUE(init);

    auto expr = dynamic_cast<SubscriptExpression*>(init->expression.get());
    
    EXPECT_TRUE(expr);
}

TEST_F(ParserTest, ParseFunctionCallExpression_NoArgs)
{
    auto ast = parse_string("int main(void) { return func(); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto call_expr = dynamic_cast<FunctionCallExpression*>(return_stmt->expression.get());

    ASSERT_NE(call_expr, nullptr);
    EXPECT_EQ(call_expr->name.name, "func");
    EXPECT_EQ(call_expr->arguments.size(), 0);
}

TEST_F(ParserTest, ParseFunctionCallExpression_SingleArg)
{
    auto ast = parse_string("int main(void) { return func(42); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto call_expr = dynamic_cast<FunctionCallExpression*>(return_stmt->expression.get());

    ASSERT_NE(call_expr, nullptr);
    EXPECT_EQ(call_expr->name.name, "func");
    EXPECT_EQ(call_expr->arguments.size(), 1);

    auto const_expr = dynamic_cast<ConstantExpression*>(call_expr->arguments[0].get());
    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<int>(const_expr->value), 42);
}

TEST_F(ParserTest, ParseFunctionCallExpression_MultipleArgs)
{
    auto ast = parse_string("int main(void) { return func(a, b + 1, c * 2); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto call_expr = dynamic_cast<FunctionCallExpression*>(return_stmt->expression.get());

    ASSERT_NE(call_expr, nullptr);
    EXPECT_EQ(call_expr->name.name, "func");
    EXPECT_EQ(call_expr->arguments.size(), 3);

    // First argument: variable 'a'
    auto var_expr = dynamic_cast<VariableExpression*>(call_expr->arguments[0].get());
    ASSERT_NE(var_expr, nullptr);
    EXPECT_EQ(var_expr->identifier.name, "a");

    // Second argument: binary expression 'b + 1'
    auto add_expr = dynamic_cast<BinaryExpression*>(call_expr->arguments[1].get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    // Third argument: binary expression 'c * 2'
    auto mult_expr = dynamic_cast<BinaryExpression*>(call_expr->arguments[2].get());
    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);
}

TEST_F(ParserTest, ParseNestedFunctionCalls)
{
    auto ast = parse_string("int main(void) { return func1(func2(x), func3()); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto outer_call = dynamic_cast<FunctionCallExpression*>(return_stmt->expression.get());

    ASSERT_NE(outer_call, nullptr);
    EXPECT_EQ(outer_call->name.name, "func1");
    EXPECT_EQ(outer_call->arguments.size(), 2);

    // First argument should be func2(x)
    auto inner_call1 = dynamic_cast<FunctionCallExpression*>(outer_call->arguments[0].get());
    ASSERT_NE(inner_call1, nullptr);
    EXPECT_EQ(inner_call1->name.name, "func2");
    EXPECT_EQ(inner_call1->arguments.size(), 1);

    // Second argument should be func3()
    auto inner_call2 = dynamic_cast<FunctionCallExpression*>(outer_call->arguments[1].get());
    ASSERT_NE(inner_call2, nullptr);
    EXPECT_EQ(inner_call2->name.name, "func3");
    EXPECT_EQ(inner_call2->arguments.size(), 0);
}

// ============== Missing Constant Type Tests ==============

TEST_F(ParserTest, ParseUnsignedConstant)
{
    auto ast = parse_string("unsigned int main(void) { return 42U; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto const_expr = dynamic_cast<ConstantExpression*>(return_stmt->expression.get());

    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<unsigned int>(const_expr->value), 42U);
}

TEST_F(ParserTest, ParseLongConstant)
{
    auto ast = parse_string("long main(void) { return 42L; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto const_expr = dynamic_cast<ConstantExpression*>(return_stmt->expression.get());

    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<long>(const_expr->value), 42L);
}

TEST_F(ParserTest, ParseUnsignedLongConstant)
{
    auto ast = parse_string("unsigned long main(void) { return 42UL; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto const_expr = dynamic_cast<ConstantExpression*>(return_stmt->expression.get());

    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<unsigned long>(const_expr->value), 42UL);
}

TEST_F(ParserTest, ParseDoubleConstant)
{
    auto ast = parse_string("double main(void) { return 3.14; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto const_expr = dynamic_cast<ConstantExpression*>(return_stmt->expression.get());

    ASSERT_NE(const_expr, nullptr);
    EXPECT_EQ(std::get<double>(const_expr->value), 3.14);
}

// ============== Missing Binary Operator Tests ==============

TEST_F(ParserTest, ParseSubtractionOperator)
{
    auto ast = parse_string("int main(void) { return 10 - 3; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::SUBTRACT);

    auto left = dynamic_cast<ConstantExpression*>(binary_expr->left_expression.get());
    auto right = dynamic_cast<ConstantExpression*>(binary_expr->right_expression.get());

    ASSERT_NE(left, nullptr);
    ASSERT_NE(right, nullptr);
    EXPECT_EQ(std::get<int>(left->value), 10);
    EXPECT_EQ(std::get<int>(right->value), 3);
}

TEST_F(ParserTest, ParseDivisionOperator)
{
    auto ast = parse_string("int main(void) { return 15 / 3; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::DIVIDE);
}

TEST_F(ParserTest, ParseRemainderOperator)
{
    auto ast = parse_string("int main(void) { return 10 % 3; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::REMAINDER);
}

TEST_F(ParserTest, ParseGreaterThanOperator)
{
    auto ast = parse_string("int main(void) { return x > y; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::GREATER_THAN);
}

TEST_F(ParserTest, ParseLessThanOperator)
{
    auto ast = parse_string("int main(void) { return x < y; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto binary_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());

    ASSERT_NE(binary_expr, nullptr);
    EXPECT_EQ(binary_expr->binary_operator, BinaryOperator::LESS_THAN);
}

// ============== Multiple Array Subscript Tests ==============

TEST_F(ParserTest, ParseMultipleSubscriptExpression)
{
    auto ast = parse_string("int main(void) { return arr[i][j]; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Should be arr[i][j] -> (arr[i])[j]
    auto outer_subscript = dynamic_cast<SubscriptExpression*>(return_stmt->expression.get());
    ASSERT_NE(outer_subscript, nullptr);

    // The first expression should be arr[i]
    auto inner_subscript = dynamic_cast<SubscriptExpression*>(outer_subscript->expression1.get());
    ASSERT_NE(inner_subscript, nullptr);

    // arr[i] should have 'arr' as expression1
    auto arr_var = dynamic_cast<VariableExpression*>(inner_subscript->expression1.get());
    ASSERT_NE(arr_var, nullptr);
    EXPECT_EQ(arr_var->identifier.name, "arr");

    // arr[i] should have 'i' as expression2
    auto i_var = dynamic_cast<VariableExpression*>(inner_subscript->expression2.get());
    ASSERT_NE(i_var, nullptr);
    EXPECT_EQ(i_var->identifier.name, "i");

    // arr[i][j] should have 'j' as expression2
    auto j_var = dynamic_cast<VariableExpression*>(outer_subscript->expression2.get());
    ASSERT_NE(j_var, nullptr);
    EXPECT_EQ(j_var->identifier.name, "j");
}

TEST_F(ParserTest, ParseTripleSubscriptExpression)
{
    auto ast = parse_string("int main(void) { return matrix[x][y][z]; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Should be matrix[x][y][z] -> ((matrix[x])[y])[z]
    auto outermost = dynamic_cast<SubscriptExpression*>(return_stmt->expression.get());
    ASSERT_NE(outermost, nullptr);

    auto middle = dynamic_cast<SubscriptExpression*>(outermost->expression1.get());
    ASSERT_NE(middle, nullptr);

    auto innermost = dynamic_cast<SubscriptExpression*>(middle->expression1.get());
    ASSERT_NE(innermost, nullptr);

    // Check the base variable
    auto matrix_var = dynamic_cast<VariableExpression*>(innermost->expression1.get());
    ASSERT_NE(matrix_var, nullptr);
    EXPECT_EQ(matrix_var->identifier.name, "matrix");
}

// ============== Complex Cast Expression Tests ==============

TEST_F(ParserTest, ParseArrayCastExpression)
{
    auto ast = parse_string("int* y = (int[10]) x;");

    auto var_decl = dynamic_cast<VariableDeclaration*>(ast->declarations[0].get());
    ASSERT_NE(var_decl, nullptr);

    auto init = dynamic_cast<SingleInitializer*>(var_decl->expression.value().get());
    auto cast_expr = dynamic_cast<CastExpression*>(init->expression.get());

    ASSERT_NE(cast_expr, nullptr);
    EXPECT_TRUE(is_type<ArrayType>(*cast_expr->target_type));
}

TEST_F(ParserTest, ParseNestedCastExpression)
{
    auto ast = parse_string("int main(void) { return (int)(long)x; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Outer cast to int
    auto outer_cast = dynamic_cast<CastExpression*>(return_stmt->expression.get());
    ASSERT_NE(outer_cast, nullptr);
    EXPECT_TRUE(is_type<IntType>(*outer_cast->target_type));

    // Inner cast to long
    auto inner_cast = dynamic_cast<CastExpression*>(outer_cast->expression.get());
    ASSERT_NE(inner_cast, nullptr);
    EXPECT_TRUE(is_type<LongType>(*inner_cast->target_type));
}

// ============== Complex Conditional Expression Tests ==============

TEST_F(ParserTest, ParseNestedConditionalExpression)
{
    auto ast = parse_string("int main(void) { return a ? b ? c : d : e; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Should be a ? (b ? c : d) : e due to right associativity
    auto outer_cond = dynamic_cast<ConditionalExpression*>(return_stmt->expression.get());
    ASSERT_NE(outer_cond, nullptr);

    // Check that condition is 'a'
    auto a_var = dynamic_cast<VariableExpression*>(outer_cond->condition.get());
    ASSERT_NE(a_var, nullptr);
    EXPECT_EQ(a_var->identifier.name, "a");

    // Check that true_expression is 'b ? c : d'
    auto inner_cond = dynamic_cast<ConditionalExpression*>(outer_cond->true_expression.get());
    ASSERT_NE(inner_cond, nullptr);

    // Check that false_expression is 'e'
    auto e_var = dynamic_cast<VariableExpression*>(outer_cond->false_expression.get());
    ASSERT_NE(e_var, nullptr);
    EXPECT_EQ(e_var->identifier.name, "e");
}

TEST_F(ParserTest, ParseConditionalWithComplexExpressions)
{
    auto ast = parse_string("int main(void) { return (a + b) > 0 ? func(x, y) : arr[i]; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto cond_expr = dynamic_cast<ConditionalExpression*>(return_stmt->expression.get());

    ASSERT_NE(cond_expr, nullptr);

    // Condition should be (a + b) > 0
    auto condition = dynamic_cast<BinaryExpression*>(cond_expr->condition.get());
    ASSERT_NE(condition, nullptr);
    EXPECT_EQ(condition->binary_operator, BinaryOperator::GREATER_THAN);

    // True expression should be func(x, y)
    auto true_expr = dynamic_cast<FunctionCallExpression*>(cond_expr->true_expression.get());
    ASSERT_NE(true_expr, nullptr);
    EXPECT_EQ(true_expr->name.name, "func");

    // False expression should be arr[i]
    auto false_expr = dynamic_cast<SubscriptExpression*>(cond_expr->false_expression.get());
    ASSERT_NE(false_expr, nullptr);
}

// ============== Advanced Precedence and Associativity Tests ==============

TEST_F(ParserTest, ParseOperatorPrecedenceComplex)
{
    auto ast = parse_string("int main(void) { return a * b + c / d - e % f; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Should be ((a * b) + (c / d)) - (e % f)
    auto outer_sub = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());
    ASSERT_NE(outer_sub, nullptr);
    EXPECT_EQ(outer_sub->binary_operator, BinaryOperator::SUBTRACT);

    auto left_add = dynamic_cast<BinaryExpression*>(outer_sub->left_expression.get());
    ASSERT_NE(left_add, nullptr);
    EXPECT_EQ(left_add->binary_operator, BinaryOperator::ADD);

    auto right_mod = dynamic_cast<BinaryExpression*>(outer_sub->right_expression.get());
    ASSERT_NE(right_mod, nullptr);
    EXPECT_EQ(right_mod->binary_operator, BinaryOperator::REMAINDER);
}

TEST_F(ParserTest, ParseMixedUnaryAndBinary)
{
    auto ast = parse_string("int main(void) { return -a * ~b + !c; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Should be ((-a) * (~b)) + (!c)
    auto add_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    auto mult_expr = dynamic_cast<BinaryExpression*>(add_expr->left_expression.get());
    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);

    // Left side of multiplication should be -a
    auto negate_expr = dynamic_cast<UnaryExpression*>(mult_expr->left_expression.get());
    ASSERT_NE(negate_expr, nullptr);
    EXPECT_EQ(negate_expr->unary_operator, UnaryOperator::NEGATE);

    // Right side of multiplication should be ~b
    auto complement_expr = dynamic_cast<UnaryExpression*>(mult_expr->right_expression.get());
    ASSERT_NE(complement_expr, nullptr);
    EXPECT_EQ(complement_expr->unary_operator, UnaryOperator::COMPLEMENT);

    // Right side of addition should be !c
    auto not_expr = dynamic_cast<UnaryExpression*>(add_expr->right_expression.get());
    ASSERT_NE(not_expr, nullptr);
    EXPECT_EQ(not_expr->unary_operator, UnaryOperator::NOT);
}

// ============== Complex Expression Combinations ==============

TEST_F(ParserTest, ParseComplexExpressionWithEverything)
{
    auto ast = parse_string("int main(void) { return func(arr[i], *ptr) + (x > 0 ? y : z) * (long)w; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    // Top level should be addition
    auto add_expr = dynamic_cast<BinaryExpression*>(return_stmt->expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    // Left side should be function call
    auto func_call = dynamic_cast<FunctionCallExpression*>(add_expr->left_expression.get());
    ASSERT_NE(func_call, nullptr);
    EXPECT_EQ(func_call->name.name, "func");
    EXPECT_EQ(func_call->arguments.size(), 2);

    // First argument should be arr[i]
    auto subscript = dynamic_cast<SubscriptExpression*>(func_call->arguments[0].get());
    ASSERT_NE(subscript, nullptr);

    // Second argument should be *ptr
    auto deref = dynamic_cast<DereferenceExpression*>(func_call->arguments[1].get());
    ASSERT_NE(deref, nullptr);

    // Right side should be multiplication
    auto mult_expr = dynamic_cast<BinaryExpression*>(add_expr->right_expression.get());
    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);

    // Left side of multiplication should be conditional
    auto cond_expr = dynamic_cast<ConditionalExpression*>(mult_expr->left_expression.get());
    ASSERT_NE(cond_expr, nullptr);

    // Right side of multiplication should be cast
    auto cast_expr = dynamic_cast<CastExpression*>(mult_expr->right_expression.get());
    ASSERT_NE(cast_expr, nullptr);
}

// ============== Additional Edge Cases ==============

TEST_F(ParserTest, ParseComplexAddressOfExpression)
{
    auto ast = parse_string("int main(void) { return &arr[i][j]; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    auto addr_expr = dynamic_cast<AddressOfExpression*>(return_stmt->expression.get());
    ASSERT_NE(addr_expr, nullptr);

    // Should be address of arr[i][j]
    auto subscript = dynamic_cast<SubscriptExpression*>(addr_expr->expression.get());
    ASSERT_NE(subscript, nullptr);
}

TEST_F(ParserTest, ParseComplexDereferenceExpression)
{
    auto ast = parse_string("int main(void) { return *(ptr + 1); }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    
    auto deref_expr = dynamic_cast<DereferenceExpression*>(return_stmt->expression.get());
    ASSERT_NE(deref_expr, nullptr);

    // Should be dereference of (ptr + 1)
    auto add_expr = dynamic_cast<BinaryExpression*>(deref_expr->expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);
}

TEST_F(ParserTest, ParseComplexAssignmentExpression)
{
    auto ast = parse_string("int main(void) { arr[i] = func(x) + y; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(func_decl->body.value()->items[0].get());
    auto assign_expr = dynamic_cast<AssignmentExpression*>(expr_stmt->expression.get());

    ASSERT_NE(assign_expr, nullptr);

    // Left side should be arr[i]
    auto subscript = dynamic_cast<SubscriptExpression*>(assign_expr->left_expression.get());
    ASSERT_NE(subscript, nullptr);

    // Right side should be func(x) + y
    auto add_expr = dynamic_cast<BinaryExpression*>(assign_expr->right_expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    // Left side of addition should be function call
    auto func_call = dynamic_cast<FunctionCallExpression*>(add_expr->left_expression.get());
    ASSERT_NE(func_call, nullptr);
}

TEST_F(ParserTest, ParseArraySubscriptWithExpression)
{
    auto ast = parse_string("int main(void) { return arr[i + j * 2]; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto return_stmt = dynamic_cast<ReturnStatement*>(func_decl->body.value()->items[0].get());
    auto subscript = dynamic_cast<SubscriptExpression*>(return_stmt->expression.get());

    ASSERT_NE(subscript, nullptr);

    // Index should be i + j * 2
    auto add_expr = dynamic_cast<BinaryExpression*>(subscript->expression2.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    // Right side should be j * 2
    auto mult_expr = dynamic_cast<BinaryExpression*>(add_expr->right_expression.get());
    ASSERT_NE(mult_expr, nullptr);
    EXPECT_EQ(mult_expr->binary_operator, BinaryOperator::MULTIPLY);
}

TEST_F(ParserTest, ParseChainedAssignmentWithComplexExpressions)
{
    auto ast = parse_string("int main(void) { x = y = func(z) + 1; }");

    auto func_decl = dynamic_cast<FunctionDeclaration*>(ast->declarations[0].get());
    auto expr_stmt = dynamic_cast<ExpressionStatement*>(func_decl->body.value()->items[0].get());
    auto assign1 = dynamic_cast<AssignmentExpression*>(expr_stmt->expression.get());

    ASSERT_NE(assign1, nullptr);

    // Second assignment: y = func(z) + 1
    auto assign2 = dynamic_cast<AssignmentExpression*>(assign1->right_expression.get());
    ASSERT_NE(assign2, nullptr);

    // Right side should be func(z) + 1
    auto add_expr = dynamic_cast<BinaryExpression*>(assign2->right_expression.get());
    ASSERT_NE(add_expr, nullptr);
    EXPECT_EQ(add_expr->binary_operator, BinaryOperator::ADD);

    // Left side of addition should be function call
    auto func_call = dynamic_cast<FunctionCallExpression*>(add_expr->left_expression.get());
    ASSERT_NE(func_call, nullptr);
    EXPECT_EQ(func_call->name.name, "func");
}