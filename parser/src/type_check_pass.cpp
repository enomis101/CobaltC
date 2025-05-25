#include "parser/type_check_pass.h"
#include "parser/parser_ast.h"
#include "parser/symbol_table.h"
#include <format>
#include <variant>

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

void TypeCheckPass::visit(FunctionDeclaration& function_declaration)
{
    std::unique_ptr<FunctionType> fun_type = std::make_unique<FunctionType>(function_declaration.params.size());
    bool has_body = function_declaration.body.has_value();
    bool already_defined = false;
    const std::string& function_name = function_declaration.name.name;
    bool global = function_declaration.storage_class != StorageClass::STATIC;

    if (m_symbol_table.symbols().contains(function_name)) {
        SymbolTable::Entry& prev_decl = m_symbol_table.symbols().at(function_name);
        FunctionType* prev_fun_type = dynamic_cast<FunctionType*>(prev_decl.type.get());

        if (*prev_fun_type != *fun_type) {
            throw TypeCheckPassError(std::format("Incompatible function declaration of {}", function_name));
        }
        already_defined = std::get<FunctionAttribute>(prev_decl.attribute).defined;
        if (already_defined && has_body) {
            throw TypeCheckPassError(std::format("Function {} defined more than once", function_name));
        }
        if (std::get<FunctionAttribute>(prev_decl.attribute).global && !global) {
            throw TypeCheckPassError(std::format("Function {} declared as static follows a non-static declaration", function_name));
        }
        global = std::get<FunctionAttribute>(prev_decl.attribute).global;
    }

    bool defined = (already_defined || has_body);
    m_symbol_table.symbols().insert_or_assign(function_name, SymbolTable::Entry(std::move(fun_type), FunctionAttribute(defined, global)));

    for (auto& param : function_declaration.params) {
        resolve_function_param_declaration(param);
    }

    if (function_declaration.body.has_value()) {
        function_declaration.body.value()->accept(*this);
    }
}

void TypeCheckPass::visit(VariableDeclaration& node)
{
    if (m_symbol_table.is_top_level(node)) {
        typecheck_file_scope_variable_declaration(node);
    } else {
        typecheck_local_variable_declaration(node);
    }
}

void TypeCheckPass::visit(Program& node)
{
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void TypeCheckPass::visit(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    auto& type = m_symbol_table.symbols().at(variable_name).type;
    if (!dynamic_cast<IntType*>(type.get())) {
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
    if (node.declaration->storage_class != StorageClass::NONE) {
        throw TypeCheckPassError("In TypeCheckPass: a variable declaration in a for loop cannot have a storage class");
    }
    node.declaration->accept(*this);
}

void TypeCheckPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void TypeCheckPass::resolve_function_param_declaration(Identifier& identifier)
{
    m_symbol_table.symbols().insert(std::make_pair(identifier.name, SymbolTable::Entry(std::make_unique<IntType>(), LocalAttribute {})));
}

void TypeCheckPass::typecheck_file_scope_variable_declaration(VariableDeclaration& variable_declaration)
{
    const std::string& variable_name = variable_declaration.identifier.name;
    Initializer initial_value;
    if (!variable_declaration.expression.has_value()) {
        if (variable_declaration.storage_class == StorageClass::EXTERN) {
            initial_value = NoInit {};
        } else {
            initial_value = TentativeInit {};
        }
    } else if (ConstantExpression* expr = dynamic_cast<ConstantExpression*>(variable_declaration.expression.value().get())) {
        initial_value = InitialValue { expr->value };
    } else {
        throw TypeCheckPassError(std::format("In typecheck_file_scope_variable_declaration: file-scope variable declaration of {} has non-constant initializer!", variable_name));
    }

    bool global = variable_declaration.storage_class != StorageClass::STATIC;
    if (m_symbol_table.symbols().contains(variable_name)) {

        auto& old_decl = m_symbol_table.symbols().at(variable_name);
        if (!std::holds_alternative<StaticAttribute>(old_decl.attribute)) {
            throw TypeCheckPassError(std::format("In typecheck_file_scope_variable_declaration: prev. file scope variable declaration of {} does not have a StaticAttribute!", variable_name));
        }
        StaticAttribute& old_attr = std::get<StaticAttribute>(old_decl.attribute);
        if (!dynamic_cast<IntType*>(old_decl.type.get())) {
            throw TypeCheckPassError(std::format("In typecheck_file_scope_variable_declaration: function {} declared as variable!", variable_name));
        }
        if (variable_declaration.storage_class == StorageClass::EXTERN) {
            global = old_attr.global;
        } else if (old_attr.global != global) {
            throw TypeCheckPassError(std::format("In typecheck_file_scope_variable_declaration: conflicting variable linkage for {}", variable_name));
        }

        // Check prev initialization
        if (std::holds_alternative<InitialValue>(old_attr.init)) {
            if (std::holds_alternative<InitialValue>(initial_value)) {
                throw TypeCheckPassError(std::format("In typecheck_file_scope_variable_declaration: conflicting file scope variable definitions for {}", variable_name));
            } else {
                initial_value = old_attr.init;
            }
        } else if (std::holds_alternative<InitialValue>(initial_value) && std::holds_alternative<TentativeInit>(old_attr.init)) {
            initial_value = TentativeInit {};
        }
    }
    StaticAttribute attr(initial_value, global);
    m_symbol_table.symbols().insert_or_assign(variable_name, SymbolTable::Entry(std::make_unique<IntType>(), attr));
}
void TypeCheckPass::typecheck_local_variable_declaration(VariableDeclaration& variable_declaration)
{
    const std::string& variable_name = variable_declaration.identifier.name;
    if (variable_declaration.storage_class == StorageClass::EXTERN) {
        if (variable_declaration.expression.has_value()) {
            throw TypeCheckPassError(std::format("In typecheck_local_variable_declaration: Initializer on local extern variable declaration for {}", variable_name));
        }
        if (m_symbol_table.symbols().contains(variable_name)) {
            auto& old_decl = m_symbol_table.symbols().at(variable_name);
            if (!dynamic_cast<IntType*>(old_decl.type.get())) {
                throw TypeCheckPassError(std::format("In typecheck_local_variable_declaration: function {} declared as local variable!", variable_name));
            }
            // a local extern declaration will never change the initial value or linkage we have already recorded
        } else {
            m_symbol_table.symbols().insert(std::make_pair(variable_name, SymbolTable::Entry(std::make_unique<IntType>(), StaticAttribute { NoInit {}, true })));
        }
    } else if (variable_declaration.storage_class == StorageClass::STATIC) {
        Initializer initial_value;
        if (!variable_declaration.expression.has_value()) {
            initial_value = InitialValue { 0 };
        } else if (ConstantExpression* expr = dynamic_cast<ConstantExpression*>(variable_declaration.expression.value().get())) {
            initial_value = InitialValue { expr->value };
        } else {
            throw TypeCheckPassError(std::format("In typecheck_local_variable_declaration: local variable declaration of {} has non-constant initializer!", variable_name));
        }
        m_symbol_table.symbols().insert(std::make_pair(variable_name, SymbolTable::Entry(std::make_unique<IntType>(), StaticAttribute { initial_value, false })));
    } else { // local variable
        m_symbol_table.symbols().insert(std::make_pair(variable_name, SymbolTable::Entry(std::make_unique<IntType>(), LocalAttribute {})));
        if (variable_declaration.expression.has_value()) {
            variable_declaration.expression.value()->accept(*this);
        }
    }
}
