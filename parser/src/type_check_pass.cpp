#include "parser/type_check_pass.h"
#include <format>

using namespace parser;

void TypeCheckPass::run()
{
    m_symbol_table.symbols().clear(); // clear symbol table to be sure, this should be the first pass that fills it
    m_ast->accept(*this);
}

void TypeCheckPass::visit(Identifier& node)
{
    // Empty method
}

void TypeCheckPass::visit(UnaryExpression& node)
{
    node.expression->accept(*this);
}

void TypeCheckPass::visit(BinaryExpression& node)
{
    node.left_expression->accept(*this);
    node.right_expression->accept(*this);
}

void TypeCheckPass::visit(ConstantExpression& node)
{
    // Empty method
}

void TypeCheckPass::visit(ReturnStatement& node)
{
    node.expression->accept(*this);
}

void TypeCheckPass::visit(FunctionDeclaration& node)
{
    std::unique_ptr<FunctionType> fun_type = std::make_unique<FunctionType>(node.params.size());
    bool has_body = node.body.has_value();
    bool already_defined = false;
    const std::string& function_name = node.name.name;
    if (m_symbol_table.symbols().contains(function_name)) {
        SymbolTable::Entry& prev_decl = m_symbol_table.symbols().at(function_name);
        FunctionType* prev_fun_type = dynamic_cast<FunctionType*>(prev_decl.type.get());

        if (*prev_fun_type != *fun_type) {
            throw TypeCheckPassError(std::format("Incompatible function declaration of {}", function_name));
        }
        already_defined = prev_decl.defined;
        if (already_defined && has_body) {
            throw TypeCheckPassError(std::format("Function {} defined more than once", function_name));
        }
    }

    m_symbol_table.symbols().insert_or_assign(function_name, SymbolTable::Entry(std::move(fun_type), (already_defined || has_body)));

    for (auto& param : node.params) {
        resolve_variable_declaration(param);
    }

    if (node.body.has_value()) {
        node.body.value()->accept(*this);
    }
}

void TypeCheckPass::visit(VariableDeclaration& node)
{
    resolve_variable_declaration(node.identifier);

    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void TypeCheckPass::visit(Program& node)
{
    for (auto& fun_decl : node.functions) {
        fun_decl->accept(*this);
    }
}

void TypeCheckPass::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    auto& type = m_symbol_table.symbols().at(variable_name).type;
    if (!dynamic_cast<PrimitiveType*>(type.get())) {
        throw TypeCheckPassError(std::format("Function name {} used as variable", variable_name));
    }
}

void TypeCheckPass::visit(AssignmentExpression& node)
{
    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw TypeCheckPassError("Invalid lvalue!");
    }

    node.left_expression->accept(*this);
    node.right_expression->accept(*this);
}

void TypeCheckPass::visit(ConditionalExpression& node)
{
    node.condition->accept(*this);

    node.true_expression->accept(*this);

    node.false_expression->accept(*this);
}

void TypeCheckPass::visit(FunctionCallExpression& node)
{
    std::string& function_name = node.name.name;
    auto& type = m_symbol_table.symbols().at(function_name).type;
    if (!dynamic_cast<FunctionType*>(type.get())) {
        throw TypeCheckPassError(std::format("Variable {} used as function name", function_name));
    }
    FunctionType* fun_type = dynamic_cast<FunctionType*>(type.get());
    if (fun_type->parameters_count != node.arguments.size()) {
        throw TypeCheckPassError(std::format("Function {} called with the wrong number of parameters {} expected {} ", function_name, node.arguments.size(), fun_type->parameters_count));
    }

    // Visit arguments
    for (auto& arg : node.arguments) {
        arg->accept(*this);
    }
}

void TypeCheckPass::visit(ExpressionStatement& node)
{
    node.expression->accept(*this);
}

void TypeCheckPass::visit(IfStatement& node)
{
    node.condition->accept(*this);

    node.then_statement->accept(*this);

    if (node.else_statement.has_value()) {
        node.else_statement.value()->accept(*this);
    }
}

void TypeCheckPass::visit(NullStatement& node)
{
    // Empty method
}

void TypeCheckPass::visit(Block& node)
{
    for (auto& item : node.items) {
        item->accept(*this);
    }
}

void TypeCheckPass::visit(CompoundStatement& node)
{
    node.block->accept(*this);
}

void TypeCheckPass::visit(WhileStatement& node)
{
    node.condition->accept(*this);
    node.body->accept(*this);
}

void TypeCheckPass::visit(DoWhileStatement& node)
{
    node.condition->accept(*this);
    node.body->accept(*this);
}

void TypeCheckPass::visit(ForStatement& node)
{
    node.init->accept(*this);

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.post.has_value()) {
        node.post.value()->accept(*this);
    }

    node.body->accept(*this);
}

void TypeCheckPass::visit(ForInitDeclaration& node)
{
    node.declaration->accept(*this);
}

void TypeCheckPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void TypeCheckPass::resolve_variable_declaration(Identifier& identifier)
{
    m_symbol_table.symbols().insert(std::make_pair(identifier.name, SymbolTable::Entry(std::make_unique<PrimitiveType>(PrimitiveTypeEnum::INT))));
}
