#include "parser/identifier_resolution_pass.h"
#include "parser/parser_ast.h"
#include "parser/semantic_analyzer_error.h"
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
            throw SemanticAnalyzerError(this, std::format("Function declaration {} already declared with no linkage (local variable)", function_name));
        }
    }
    m_identifier_map.insert_or_assign(function_name, MapEntry(function_name, true, true));
    IdentifierMapGuard copy_guard(m_identifier_map); // map is copied on construction and restored on destruction
    for (auto& param : node.params) {
        resolve_variable_identifier(param);
    }

    if (node.scope == DeclarationScope::Block) {
        if (node.storage_class == StorageClass::STATIC) {
            throw SemanticAnalyzerError(this, std::format("Function {} at local scope has static specifier", function_name));
        }
    }

    if (node.body.has_value()) {
        if (node.scope == DeclarationScope::Block) {
            throw SemanticAnalyzerError(this, std::format("Definining function {} at local scope", function_name));
        }
        node.body.value()->accept(*this);
    }
}

void IdentifierResolutionPass::visit(Program& node)
{
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void IdentifierResolutionPass::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    if (!m_identifier_map.contains(variable_name)) {
        throw SemanticAnalyzerError(this, std::format("Use of undeclared variable {}", variable_name));
    }
    variable_name = m_identifier_map.at(variable_name).new_name;
}
void IdentifierResolutionPass::visit(CastExpression& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(AssignmentExpression& node)
{
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
        throw SemanticAnalyzerError(this, std::format("Use of undeclared function {}", function_name));
    }

    function_name = m_identifier_map.at(function_name).new_name;

    // Visit arguments
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void IdentifierResolutionPass::visit(DereferenceExpression& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(AddressOfExpression& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(ExpressionStatement& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(SubscriptExpression& node)
{
    node.expression1->accept(*this);
    node.expression2->accept(*this);
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

void IdentifierResolutionPass::visit(SingleInitializer& node)
{
    node.expression->accept(*this);
}

void IdentifierResolutionPass::visit(CompoundInitializer& node)
{
    for (auto& init : node.initializer_list) {
        init->accept(*this);
    }
}

void IdentifierResolutionPass::visit(VariableDeclaration& node)
{
    if (node.scope == DeclarationScope::File) {
        resolve_file_scope_variable_declaration(node);
    } else {
        resolve_local_variable_declaration(node);
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

void IdentifierResolutionPass::resolve_variable_identifier(Identifier& identifier)
{
    const std::string& variable_name = identifier.name;
    if (m_identifier_map.contains(variable_name) && m_identifier_map.at(variable_name).from_current_scope) { // throw error only if the other declaration is from the same block
        throw SemanticAnalyzerError(this, std::format("Duplicate variable declaration: {}", variable_name));
    }

    std::string new_name = m_name_generator->make_temporary(variable_name);
    m_identifier_map.insert_or_assign(variable_name, MapEntry(new_name, true, false));
    identifier.name = new_name;
}

void IdentifierResolutionPass::resolve_file_scope_variable_declaration(VariableDeclaration& var_decl)
{
    const std::string& var_name = var_decl.identifier.name;
    // We dont need to rename it or check previous declarations, other conflicts will be detected during Type Check stage
    m_identifier_map.insert_or_assign(var_name, MapEntry(var_name, true, true));
}

void IdentifierResolutionPass::resolve_local_variable_declaration(VariableDeclaration& var_decl)
{
    const std::string& variable_name = var_decl.identifier.name;
    if (m_identifier_map.contains(variable_name)) {
        const auto& prev_decl = m_identifier_map.at(variable_name);
        if (prev_decl.from_current_scope) {
            if (!(prev_decl.has_linkage && var_decl.storage_class == StorageClass::EXTERN)) {
                throw SemanticAnalyzerError(this, std::format("Conflicting local declaration of: {}", variable_name));
            }
        }
    }

    if (var_decl.storage_class == StorageClass::EXTERN) { //  Declaration has linkage
        m_identifier_map.insert_or_assign(variable_name, MapEntry(variable_name, true, true));
        // Do not check initializer handled by type check
    } else {
        resolve_variable_identifier(var_decl.identifier); // Static variable have no linkage and should be renamed
        if (var_decl.expression.has_value()) {
            var_decl.expression.value()->accept(*this);
        }
    }
}

void IdentifierResolutionPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}
