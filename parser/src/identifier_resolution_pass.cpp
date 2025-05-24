#include "parser/identifier_resolution_pass.h"
#include <format>

using namespace parser;

void IdentifierResolutionPass::run()
{
    m_identifier_map.clear();
    m_ast->accept(*this);
}

void IdentifierResolutionPass::visit(Identifier& node)
{
    // Empty method
}

void IdentifierResolutionPass::visit(UnaryExpression& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(BinaryExpression& node)
{
    node.left_expression->accept(*this);
    node.right_expression->accept(*this);
}

void IdentifierResolutionPass::visit(ConstantExpression& node)
{
    // Empty method
}

void IdentifierResolutionPass::visit(ReturnStatement& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(FunctionDeclaration& node)
{
    std::string& function_name = node.name.name;
    if (m_identifier_map.contains(function_name)) {
        auto& prev_entry = m_identifier_map.at(function_name);
        if (prev_entry.from_current_scope && !prev_entry.has_linkage) {
            throw IdentifierResolutionPassError(std::format("Function declaration {} already declared with no linkage (local variable)", function_name));
        }
    }
    m_identifier_map.insert_or_assign(function_name, MapEntry(function_name, true, true));
    IdentifierMapGuard copy_guard(m_identifier_map); // map is copied on construction and restored on destruction
    for (auto& param : node.params) {
        resolve_variable_declaration(param);
    }

    if (node.body.has_value()) {
        if (!is_top_level(node)) {
            throw IdentifierResolutionPassError(std::format("Definining function {} at local scope", function_name));
        }
        node.body.value()->accept(*this);
    }
}

void IdentifierResolutionPass::visit(Program& node)
{
    // for (auto& fun_decl : node.functions) {
    //     m_top_level_tracker[fun_decl.get()] = true;
    //     fun_decl->accept(*this);
    // }
}

void IdentifierResolutionPass::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    if (!m_identifier_map.contains(variable_name)) {
        throw IdentifierResolutionPassError(std::format("Use of undeclared variable {}", variable_name));
    }
    variable_name = m_identifier_map.at(variable_name).new_name;
}

void IdentifierResolutionPass::visit(AssignmentExpression& node)
{
    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw IdentifierResolutionPassError("Invalid lvalue!");
    }

    node.left_expression->accept(*this);
    node.right_expression->accept(*this);
}

void IdentifierResolutionPass::visit(ConditionalExpression& node)
{
    node.condition->accept(*this);

    node.true_expression->accept(*this);

    node.false_expression->accept(*this);
}

void IdentifierResolutionPass::visit(FunctionCallExpression& node)
{
    std::string& function_name = node.name.name;
    if (!m_identifier_map.contains(function_name)) {
        throw IdentifierResolutionPassError(std::format("Use of undeclared function {}", function_name));
    }
    function_name = m_identifier_map.at(function_name).new_name;

    // Visit arguments
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void IdentifierResolutionPass::visit(ExpressionStatement& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(IfStatement& node)
{
    node.condition->accept(*this);

    node.then_statement->accept(*this);

    if (node.else_statement.has_value()) {
        node.else_statement.value()->accept(*this);
    }
}

void IdentifierResolutionPass::visit(NullStatement& node)
{
    // Empty method
}

void IdentifierResolutionPass::visit(VariableDeclaration& node)
{
    resolve_variable_declaration(node.identifier);

    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void IdentifierResolutionPass::visit(Block& node)
{
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void IdentifierResolutionPass::visit(CompoundStatement& node)
{
    IdentifierMapGuard copy_guard(m_identifier_map);
    node.block->accept(*this);
}

void IdentifierResolutionPass::visit(WhileStatement& node)
{
    node.condition->accept(*this);
    node.body->accept(*this);
}

void IdentifierResolutionPass::visit(DoWhileStatement& node)
{
    node.condition->accept(*this);
    node.body->accept(*this);
}

void IdentifierResolutionPass::visit(ForStatement& node)
{
    IdentifierMapGuard copy_guard(m_identifier_map); // map is copied on construction and restored on destruction

    node.init->accept(*this);

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.post.has_value()) {
        node.post.value()->accept(*this);
    }

    node.body->accept(*this);
}

void IdentifierResolutionPass::visit(ForInitDeclaration& node)
{
    node.declaration->accept(*this);
}

void IdentifierResolutionPass::resolve_variable_declaration(Identifier& identifier)
{
    const std::string& variable_name = identifier.name;
    if (m_identifier_map.contains(variable_name) && m_identifier_map.at(variable_name).from_current_scope) { // throw error only if the other declaration is from the same block
        throw IdentifierResolutionPassError(std::format("Duplicate variable declaration: {}", variable_name));
    }

    std::string new_name = m_name_generator.make_temporary(variable_name);
    m_identifier_map.insert_or_assign(variable_name, MapEntry(new_name, true, false));
    identifier.name = new_name;
}

bool IdentifierResolutionPass::is_top_level(FunctionDeclaration& fun_decl)
{
    return m_top_level_tracker.contains(&fun_decl) && m_top_level_tracker[&fun_decl];
}

void IdentifierResolutionPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}
