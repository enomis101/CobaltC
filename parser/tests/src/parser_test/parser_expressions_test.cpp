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
    parser::PrinterVisitor printer;
    printer.generate_dot_file("debug/test.dot", *(ast.get()));
    
    EXPECT_TRUE(expr);
}

// ============== TODOs ==============

// TODO: Cast expressions
// TODO: Complex type casts

// TODO: Unary operators edge cases

// TODO: Binary operators edge cases
// TODO: All comparison operators (`!=`, `<=`, `>=`) - some covered
// TODO: Modulo operator (`%`)
// TODO: All combinations and precedence

// TODO: Postfix expressions
// TODO: Array subscripting
// TODO: Function calls with complex arguments

// TODO: Right associativity edge cases
// TODO: Conditional operator (`? :`) associativity - partially covered
// TODO: Assignment operator chaining - covered

// TODO: Precedence combinations
// TODO: Mixed arithmetic, logical, and comparison operators - partially covered
// TODO: Conditional operator with complex expressions
