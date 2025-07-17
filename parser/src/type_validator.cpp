#include "parser/type_validator.h"
#include "common/error/internal_compiler_error.h"

using namespace parser;

void TypeValidator::validate_types(ParserAST& ast)
{
    ast.accept(*this);
}

void TypeValidator::validate_type(const std::unique_ptr<Type>& type, const std::string& node_name)
{
    if (!type) {
        throw InternalCompilerError("Type must be valid for " + node_name + " after semantic analysis");
    }
}

void TypeValidator::visit(Identifier& node)
{
    // Identifiers themselves don't have types, only when used in expressions
}

void TypeValidator::visit(UnaryExpression& node)
{
    validate_type(node.type, "UnaryExpression");

    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(BinaryExpression& node)
{
    validate_type(node.type, "BinaryExpression");

    if (node.left_expression) {
        node.left_expression->accept(*this);
    }

    if (node.right_expression) {
        node.right_expression->accept(*this);
    }
}

void TypeValidator::visit(ConstantExpression& node)
{
    validate_type(node.type, "ConstantExpression");
}

void TypeValidator::visit(ReturnStatement& node)
{
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(VariableExpression& node)
{
    validate_type(node.type, "VariableExpression");

    node.identifier.accept(*this);
}

void TypeValidator::visit(CastExpression& node)
{
    validate_type(node.type, "CastExpression result_type");
    validate_type(node.target_type, "CastExpression target_type");

    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(AssignmentExpression& node)
{
    validate_type(node.type, "AssignmentExpression");

    if (node.left_expression) {
        node.left_expression->accept(*this);
    }

    if (node.right_expression) {
        node.right_expression->accept(*this);
    }
}

void TypeValidator::visit(ConditionalExpression& node)
{
    validate_type(node.type, "ConditionalExpression");

    if (node.condition) {
        node.condition->accept(*this);
    }

    if (node.true_expression) {
        node.true_expression->accept(*this);
    }

    if (node.false_expression) {
        node.false_expression->accept(*this);
    }
}

void TypeValidator::visit(FunctionCallExpression& node)
{
    validate_type(node.type, "FunctionCallExpression");

    node.name.accept(*this);

    for (const auto& argument : node.arguments) {
        if (argument) {
            argument->accept(*this);
        }
    }
}

void TypeValidator::visit(DereferenceExpression& node)
{
    validate_type(node.type, "DereferenceExpression");

    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(AddressOfExpression& node)
{
    validate_type(node.type, "AddressOfExpression");

    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(SubscriptExpression& node)
{
    validate_type(node.type, "SubscriptExpression");

    if (node.expression1) {
        node.expression1->accept(*this);
    }

    if (node.expression2) {
        node.expression2->accept(*this);
    }
}

void TypeValidator::visit(ExpressionStatement& node)
{
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(IfStatement& node)
{
    if (node.condition) {
        node.condition->accept(*this);
    }

    if (node.then_statement) {
        node.then_statement->accept(*this);
    }

    if (node.else_statement.has_value() && node.else_statement.value()) {
        node.else_statement.value()->accept(*this);
    }
}

void TypeValidator::visit(NullStatement& node)
{
    // Null statements don't have types to validate
}

void TypeValidator::visit(SingleInitializer& node)
{
    validate_type(node.type, "SingleInitializer");

    if (node.expression) {
        node.expression->accept(*this);
    }
}

void TypeValidator::visit(CompoundInitializer& node)
{
    validate_type(node.type, "CompoundInitializer");

    for (const auto& initializer : node.initializer_list) {
        if (initializer) {
            initializer->accept(*this);
        }
    }
}

void TypeValidator::visit(VariableDeclaration& node)
{
    validate_type(node.type, "VariableDeclaration");

    node.identifier.accept(*this);

    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
    }
}

void TypeValidator::visit(FunctionDeclaration& node)
{
    validate_type(node.type, "FunctionDeclaration");

    node.name.accept(*this);

    for (auto& param : node.params) {
        param.accept(*this);
    }

    if (node.body.has_value() && node.body.value()) {
        node.body.value()->accept(*this);
    }
}

void TypeValidator::visit(Block& node)
{
    for (const auto& item : node.items) {
        if (item) {
            item->accept(*this);
        }
    }
}

void TypeValidator::visit(CompoundStatement& node)
{
    if (node.block) {
        node.block->accept(*this);
    }
}

void TypeValidator::visit(Program& node)
{
    for (const auto& declaration : node.declarations) {
        if (declaration) {
            declaration->accept(*this);
        }
    }
}

void TypeValidator::visit(BreakStatement& node)
{
    if (!node.label.name.empty()) {
        node.label.accept(*this);
    }
}

void TypeValidator::visit(ContinueStatement& node)
{
    if (!node.label.name.empty()) {
        node.label.accept(*this);
    }
}

void TypeValidator::visit(WhileStatement& node)
{
    if (node.condition) {
        node.condition->accept(*this);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (!node.label.name.empty()) {
        node.label.accept(*this);
    }
}

void TypeValidator::visit(DoWhileStatement& node)
{
    if (node.body) {
        node.body->accept(*this);
    }

    if (node.condition) {
        node.condition->accept(*this);
    }

    if (!node.label.name.empty()) {
        node.label.accept(*this);
    }
}

void TypeValidator::visit(ForStatement& node)
{
    if (node.init) {
        node.init->accept(*this);
    }

    if (node.condition.has_value() && node.condition.value()) {
        node.condition.value()->accept(*this);
    }

    if (node.post.has_value() && node.post.value()) {
        node.post.value()->accept(*this);
    }

    if (node.body) {
        node.body->accept(*this);
    }

    if (!node.label.name.empty()) {
        node.label.accept(*this);
    }
}

void TypeValidator::visit(ForInitDeclaration& node)
{
    if (node.declaration) {
        node.declaration->accept(*this);
    }
}

void TypeValidator::visit(ForInitExpression& node)
{
    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
    }
}
