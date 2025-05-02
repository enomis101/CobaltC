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
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void SemanticAnalyzer::visit(BinaryExpression& node)
{
    if (node.left_expression) {
        node.left_expression->accept(*this);
    }
    if (node.right_expression) {
        node.right_expression->accept(*this);
    }
}

void SemanticAnalyzer::visit(ConstantExpression& node)
{
    // Empty method
}

void SemanticAnalyzer::visit(ReturnStatement& node)
{
    if (node.expression) {
        node.expression->accept(*this);
    }
}

void SemanticAnalyzer::visit(Function& node)
{
    if (node.name) {
        node.name->accept(*this);
    }
    for (auto& item : node.body) {
        if (item) {
            item->accept(*this);
        }
    }
}

void SemanticAnalyzer::visit(Program& node)
{
    if (node.function) {
        node.function->accept(*this);
    }
}

void SemanticAnalyzer::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    if (!m_variable_map.contains(variable_name)) {
        throw SemanticAnalyzerError(std::format("In VariableExpression Use of Undeclared Variable {}", variable_name));
    }
    variable_name = m_variable_map[variable_name];
}

void SemanticAnalyzer::visit(AssignmentExpression& node)
{
    if (node.left_expression) {
        if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
            throw SemanticAnalyzerError("In AssignmentExpression Invalid lvalue!");
        }
        node.left_expression->accept(*this);
    }
    if (node.right_expression) {
        node.right_expression->accept(*this);
    }
}

void SemanticAnalyzer::visit(ExpressionStatement& node)
{
    if (node.expression) {
        node.expression->accept(*this);
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
    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
    }
}
