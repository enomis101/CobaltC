#include "parser/variable_resolution_pass.h"
#include <format>

using namespace parser;

void VariableResolutionPass::run()
{
    m_variable_map.clear();
    m_ast->accept(*this);
}

void VariableResolutionPass::visit(Identifier& node)
{
    // Empty method
}

void VariableResolutionPass::visit(UnaryExpression& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In UnaryExpression: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void VariableResolutionPass::visit(BinaryExpression& node)
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

void VariableResolutionPass::visit(ConstantExpression& node)
{
    // Empty method
}

void VariableResolutionPass::visit(ReturnStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ReturnStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void VariableResolutionPass::visit(FunctionDeclaration& node)
{
    /*
    if (!node.name) {
        throw SemanticAnalyzerError("In FunctionDeclaration: Name pointer is null");
    }
    node.name->accept(*this);

    m_variable_map.clear();

    if (!node.body) {
        throw SemanticAnalyzerError("In FunctionDeclaration: Body pointer is null");
    }
    node.body->accept(*this);
    */
}

void VariableResolutionPass::visit(Program& node)
{
    /*
    if (!node.function) {
        throw SemanticAnalyzerError("In Program: FunctionDeclaration pointer is null");
    }
    node.function->accept(*this);
    */
}

void VariableResolutionPass::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    if (!m_variable_map.contains(variable_name)) {
        throw SemanticAnalyzerError(std::format("In VariableExpression: Use of undeclared variable {}", variable_name));
    }
    variable_name = m_variable_map.at(variable_name).unique_name;
}

void VariableResolutionPass::visit(AssignmentExpression& node)
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

void VariableResolutionPass::visit(ConditionalExpression& node)
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

void VariableResolutionPass::visit(ExpressionStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ExpressionStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void VariableResolutionPass::visit(IfStatement& node)
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

void VariableResolutionPass::visit(NullStatement& node)
{
    // Empty method
}

void VariableResolutionPass::visit(VariableDeclaration& node)
{
    std::string& variable_name = node.identifier.name;
    if (m_variable_map.contains(variable_name) && m_variable_map.at(variable_name).from_current_block) { // throw error only if the other declaration is from the same block
        throw SemanticAnalyzerError(std::format("Duplicate variable declaration: {}", variable_name));
    }
    std::string unique_name = m_name_generator.make_temporary(variable_name);
    m_variable_map.insert_or_assign(variable_name, MapEntry(unique_name, true));
    variable_name = unique_name;
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void VariableResolutionPass::visit(Block& node)
{
    for (auto& item : node.items) {
        if (!item) {
            throw SemanticAnalyzerError("In Block: item pointer is null");
        }
        item->accept(*this);
    }
}

void VariableResolutionPass::visit(CompoundStatement& node)
{
    if (!node.block) {
        throw SemanticAnalyzerError("In CompoundStatement: block pointer is null");
    }
    std::unordered_map<std::string, VariableResolutionPass::MapEntry> old_map = m_variable_map;
    m_variable_map = copy_variable_map();
    node.block->accept(*this);
    m_variable_map = old_map;
}

std::unordered_map<std::string, VariableResolutionPass::MapEntry> VariableResolutionPass::copy_variable_map()
{
    std::unordered_map<std::string, VariableResolutionPass::MapEntry> new_map = m_variable_map;
    for (auto& p : new_map) {
        p.second.from_current_block = false; // reset flags when copying
    }
    return new_map;
}

void VariableResolutionPass::visit(WhileStatement& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In WhileStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.body) {
        throw SemanticAnalyzerError("In WhileStatement: Then body pointer is null");
    }
    node.body->accept(*this);
}

void VariableResolutionPass::visit(DoWhileStatement& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In DoWhileStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.body) {
        throw SemanticAnalyzerError("In DoWhileStatement: Then body pointer is null");
    }
    node.body->accept(*this);
}

void VariableResolutionPass::visit(ForStatement& node)
{
    std::unordered_map<std::string, MapEntry> old_map = m_variable_map;
    m_variable_map = copy_variable_map();

    if (!node.init) {
        throw SemanticAnalyzerError("In ForStatement: init pointer is null");
    }
    node.init->accept(*this);

    if (node.condition.has_value()) {
        if (!node.condition.value()) {
            throw SemanticAnalyzerError("In ForStatement: condition pointer is null");
        }
        node.condition.value()->accept(*this);
    }

    if (node.post.has_value()) {
        if (!node.post.value()) {
            throw SemanticAnalyzerError("In ForStatement: post pointer is null");
        }
        node.post.value()->accept(*this);
    }

    if (!node.body) {
        throw SemanticAnalyzerError("In ForStatement: Then body pointer is null");
    }
    node.body->accept(*this);

    m_variable_map = old_map;
}

void VariableResolutionPass::visit(ForInitDeclaration& node)
{
    if (!node.declaration) {
        throw SemanticAnalyzerError("In ForInitDeclaration: declaration pointer is null");
    }
    node.declaration->accept(*this);
}

void VariableResolutionPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        if (!node.expression.value()) {
            throw SemanticAnalyzerError("In ForInitExpression: expression pointer is null");
        }
        node.expression.value()->accept(*this);
    }
}
