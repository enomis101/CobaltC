#include "parser/semantic_analyzer.h"
#include <format>

using namespace parser;

void SemanticAnalyzer::analyze()
{
    m_variable_map.clear();
    m_ast->accept(*this);
}

void SemanticAnalyzer::visit(Identifier& node)
{
    // Empty method
}

void SemanticAnalyzer::visit(UnaryExpression& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In UnaryExpression: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void SemanticAnalyzer::visit(BinaryExpression& node)
{
    if (!node.left_expression) {
        throw SemanticAnalyzerError("In BinaryExpression: Left expression pointer is null");
    }
    node.left_expression->accept(*this);

    if (!node.right_expression) {
        throw SemanticAnalyzerError("In BinaryExpression: Right expression pointer is null");
    }
    node.right_expression->accept(*this);
}

void SemanticAnalyzer::visit(ConstantExpression& node)
{
    // Empty method
}

void SemanticAnalyzer::visit(ReturnStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ReturnStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void SemanticAnalyzer::visit(Function& node)
{
    if (!node.name) {
        throw SemanticAnalyzerError("In Function: Name pointer is null");
    }
    node.name->accept(*this);

    for (auto& item : node.body) {
        if (!item) {
            throw SemanticAnalyzerError("In Function: Body item pointer is null");
        }
        item->accept(*this);
    }
}

void SemanticAnalyzer::visit(Program& node)
{
    if (!node.function) {
        throw SemanticAnalyzerError("In Program: Function pointer is null");
    }
    node.function->accept(*this);
}

void SemanticAnalyzer::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    if (!m_variable_map.contains(variable_name)) {
        throw SemanticAnalyzerError(std::format("In VariableExpression: Use of undeclared variable {}", variable_name));
    }
    variable_name = m_variable_map[variable_name];
}

void SemanticAnalyzer::visit(AssignmentExpression& node)
{
    if (!node.left_expression) {
        throw SemanticAnalyzerError("In AssignmentExpression: Left expression pointer is null");
    }

    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw SemanticAnalyzerError("In AssignmentExpression: Invalid lvalue!");
    }
    node.left_expression->accept(*this);

    if (!node.right_expression) {
        throw SemanticAnalyzerError("In AssignmentExpression: Right expression pointer is null");
    }
    node.right_expression->accept(*this);
}

void SemanticAnalyzer::visit(ConditionalExpression& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In ConditionalExpression: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.true_expression) {
        throw SemanticAnalyzerError("In ConditionalExpression: True expression pointer is null");
    }
    node.true_expression->accept(*this);

    if (!node.false_expression) {
        throw SemanticAnalyzerError("In ConditionalExpression: False expression pointer is null");
    }
    node.false_expression->accept(*this);
}

void SemanticAnalyzer::visit(ExpressionStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ExpressionStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void SemanticAnalyzer::visit(IfStatement& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In IfStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.then_statement) {
        throw SemanticAnalyzerError("In IfStatement: Then statement pointer is null");
    }
    node.then_statement->accept(*this);

    if (node.else_statement.has_value()) {
        node.else_statement.value()->accept(*this);
    }
}

void SemanticAnalyzer::visit(NullStatement& node)
{
    // Empty method
}

void SemanticAnalyzer::visit(VariableDeclaration& node)
{
    std::string& variable_name = node.identifier.name;
    if (m_variable_map.contains(variable_name)) {
        throw SemanticAnalyzerError(std::format("Duplicate variable declaration: {}", variable_name));
    }
    std::string unique_name = m_name_generator.make_temporary(variable_name);
    m_variable_map.insert(std::make_pair(variable_name, unique_name));
    variable_name = unique_name;
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}
