#include "parser/type_check_pass.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "parser/parser_ast.h"
#include <format>
#include <memory>
#include <optional>
#include <variant>

using namespace parser;

void TypeCheckPass::run()
{
    m_ast->accept(*this);
}

void TypeCheckPass::visit(Identifier& node)
{
    // Empty method
}

void TypeCheckPass::visit(UnaryExpression& node)
{
    node.expression->accept(*this);
    if (node.unary_operator == UnaryOperator::NOT) {
        // The results of expressions that evaluate to 1 or 0 to indicate true or false have type int.
        node.type = std::make_unique<IntType>();
    } else {
        node.type = node.expression->type->clone();
    }
}

void TypeCheckPass::visit(BinaryExpression& node)
{
    node.left_expression->accept(*this);
    node.right_expression->accept(*this);

    // logical orerator evaluate to int
    if (node.binary_operator == BinaryOperator::AND || node.binary_operator == BinaryOperator::OR) {
        node.type = std::make_unique<IntType>();
        return;
    }
    auto left_type = node.left_expression->type->clone();
    auto right_type = node.right_expression->type->clone();
    auto common_type = get_common_type(*left_type, *right_type);
    convert_expression_to(node.left_expression, *common_type);
    convert_expression_to(node.right_expression, *common_type);

    switch (node.binary_operator) {
    case BinaryOperator::ADD:
    case BinaryOperator::SUBTRACT:
    case BinaryOperator::MULTIPLY:
    case BinaryOperator::DIVIDE:
    case BinaryOperator::REMAINDER:
        node.type = std::move(common_type);
        break;
    default:
        node.type = std::make_unique<IntType>();
    }
}

void TypeCheckPass::visit(ConstantExpression& node)
{
    if (std::holds_alternative<int>(node.value)) {
        node.type = std::make_unique<IntType>();
    } else if (std::holds_alternative<long>(node.value)) {
        node.type = std::make_unique<LongType>();
    } else if (std::holds_alternative<std::monostate>(node.value)) {
        throw TypeCheckPassError(std::format("Unsupported ConstantExpression at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::visit(ReturnStatement& node)
{
    node.expression->accept(*this);
    assert(m_current_function_declaration);
    const auto& function_name = m_current_function_declaration->name.name;
    auto& type = m_symbol_table->symbol_at(function_name).type;
    if (FunctionType* fun_type = dynamic_cast<FunctionType*>(type.get())) {
        convert_expression_to(node.expression, *fun_type->return_type);
    } else {
        assert(false && "Invalid m_current_function_declaration pointer");
    }
}

void TypeCheckPass::visit(FunctionDeclaration& function_declaration)
{
    const auto& function_type = dynamic_cast<FunctionType*>(function_declaration.type.get());
    bool has_body = function_declaration.body.has_value();
    bool already_defined = false;
    const std::string& function_name = function_declaration.name.name;
    bool global = function_declaration.storage_class != StorageClass::STATIC;

    if (m_symbol_table->contains_symbol(function_name)) {
        SymbolTable::SymbolEntry& prev_decl = m_symbol_table->symbol_at(function_name);
        FunctionType* prev_function_type = dynamic_cast<FunctionType*>(prev_decl.type.get());

        if (!prev_function_type || !function_type->equals(*prev_function_type)) {
            throw TypeCheckPassError(std::format("Incompatible function declaration of {} at:\n{}", function_name, m_source_manager->get_source_line(function_declaration.source_location)));
        }
        already_defined = std::get<FunctionAttribute>(prev_decl.attribute).defined;
        if (already_defined && has_body) {
            throw TypeCheckPassError(std::format("Function {} defined more than once at:\n{}", function_name, m_source_manager->get_source_line(function_declaration.source_location)));
        }

        if (std::get<FunctionAttribute>(prev_decl.attribute).global && !global) {
            throw TypeCheckPassError(std::format("Function {} declared as static follows a non-static declaration at:\n{}", function_name, m_source_manager->get_source_line(function_declaration.source_location)));
        }
        global = std::get<FunctionAttribute>(prev_decl.attribute).global;
    }

    bool defined = (already_defined || has_body);
    m_symbol_table->insert_or_assign_symbol(function_name, function_type->clone(), FunctionAttribute(defined, global));

    assert(function_declaration.params.size() == function_type->parameters_type.size());

    for (size_t i = 0; i < function_declaration.params.size(); ++i) {
        const auto& param = function_declaration.params[i];
        const auto& param_type = function_type->parameters_type[i];
        m_symbol_table->insert_symbol(param.name, param_type->clone(), LocalAttribute {});
    }

    if (function_declaration.body.has_value()) {
        m_current_function_declaration = &function_declaration;
        function_declaration.body.value()->accept(*this);
    }
}

void TypeCheckPass::visit(VariableDeclaration& node)
{
    if (node.scope == DeclarationScope::File) {
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
    auto& type = m_symbol_table->symbol_at(variable_name).type;
    if (dynamic_cast<FunctionType*>(type.get())) {
        throw TypeCheckPassError(std::format("Function name {} used as variable at:\n{}", variable_name, m_source_manager->get_source_line(node.source_location)));
    }
    assert(!node.type);
    node.type = type->clone();
}

void TypeCheckPass::visit(CastExpression& node)
{
    node.expression->accept(*this);
    node.type = node.target_type->clone();
}

void TypeCheckPass::visit(AssignmentExpression& node)
{
    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw TypeCheckPassError(std::format("Invalid lvalue in AssignmentExpression at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    node.left_expression->accept(*this);
    node.right_expression->accept(*this);
    auto left_type = node.left_expression->type->clone();
    convert_expression_to(node.right_expression, *left_type);
    node.type = std::move(left_type);
}

void TypeCheckPass::visit(ConditionalExpression& node)
{
    node.condition->accept(*this);

    node.true_expression->accept(*this);

    node.false_expression->accept(*this);

    auto true_type = node.true_expression->type->clone();
    auto false_type = node.false_expression->type->clone();
    // Find common type between two branches
    auto common_type = get_common_type(*true_type, *false_type);
    // Convert both branches to the common type
    convert_expression_to(node.true_expression, *common_type);
    convert_expression_to(node.false_expression, *common_type);
    node.type = std::move(common_type);
}

void TypeCheckPass::visit(FunctionCallExpression& node)
{
    std::string& function_name = node.name.name;
    auto& type = m_symbol_table->symbol_at(function_name).type;
    if (!dynamic_cast<FunctionType*>(type.get())) {
        throw TypeCheckPassError(std::format("Variable {} used as function name at:\n{}", function_name, m_source_manager->get_source_line(node.source_location)));
    }

    FunctionType* fun_type = dynamic_cast<FunctionType*>(type.get());
    if (fun_type->parameters_type.size() != node.arguments.size()) {
        throw TypeCheckPassError(std::format("Function {} called with the wrong number of arguments {} expected {} at:\n", function_name, node.arguments.size(), fun_type->parameters_type.size(), m_source_manager->get_source_line(node.source_location)));
    }

    // Visit arguments
    for (size_t i = 0; i < fun_type->parameters_type.size(); ++i) {
        auto& arg = node.arguments[i];
        auto& arg_type = fun_type->parameters_type[i];
        arg->accept(*this);
        convert_expression_to(arg, *arg_type);
    }
    node.type = fun_type->return_type->clone();
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
        // conversion is performed at compile time
        auto con_res = convert_constant_type(expr->value, *variable_declaration.type);
        if (!con_res.has_value()) {
            throw TypeCheckPassError(std::format("Failed convert_constant_type at:\n{}",
                m_source_manager->get_source_line(variable_declaration.source_location)));
        }
        initial_value = InitialValue { con_res.value() };
    } else {
        throw TypeCheckPassError(std::format("File-scope variable declaration of {} has non-constant initializer! at:\n{}",
            variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
    }

    bool global = variable_declaration.storage_class != StorageClass::STATIC;
    if (m_symbol_table->contains_symbol(variable_name)) {

        auto& old_decl = m_symbol_table->symbol_at(variable_name);
        if (!std::holds_alternative<StaticAttribute>(old_decl.attribute)) {
            throw TypeCheckPassError(std::format("Prev. file scope variable declaration of {} does not have a StaticAttribute! at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
        }

        if (!variable_declaration.type->equals(*old_decl.type)) {
            throw TypeCheckPassError(std::format("Conflicting variable declaration at:\n{}", m_source_manager->get_source_line(variable_declaration.source_location)));
        }

        StaticAttribute& old_attr = std::get<StaticAttribute>(old_decl.attribute);
        if (variable_declaration.storage_class == StorageClass::EXTERN) {
            global = old_attr.global;
        } else if (old_attr.global != global) {
            throw TypeCheckPassError(std::format("Conflicting variable linkage for {} at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
        }

        // Check prev initialization
        if (std::holds_alternative<InitialValue>(old_attr.init)) {
            if (std::holds_alternative<InitialValue>(initial_value)) {
                throw TypeCheckPassError(std::format("Conflicting file scope variable definitions for {} at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
            } else {
                initial_value = old_attr.init;
            }
        } else if (!std::holds_alternative<InitialValue>(initial_value) && std::holds_alternative<TentativeInit>(old_attr.init)) {
            initial_value = TentativeInit {};
        }
    }
    StaticAttribute attr(initial_value, global);
    m_symbol_table->insert_or_assign_symbol(variable_name, variable_declaration.type->clone(), attr);
}

void TypeCheckPass::typecheck_local_variable_declaration(VariableDeclaration& variable_declaration)
{
    const std::string& variable_name = variable_declaration.identifier.name;
    if (variable_declaration.storage_class == StorageClass::EXTERN) {
        if (variable_declaration.expression.has_value()) {
            throw TypeCheckPassError(std::format("Initializer on local extern variable declaration for {} at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
        }
        if (m_symbol_table->contains_symbol(variable_name)) {
            auto& old_decl = m_symbol_table->symbol_at(variable_name);

            if (!variable_declaration.type->equals(*old_decl.type)) {
                throw TypeCheckPassError(std::format("Conflicting variable declaration at:\n{}", m_source_manager->get_source_line(variable_declaration.source_location)));
            }
            // a local extern declaration will never change the initial value or linkage we have already recorded
        } else {
            m_symbol_table->insert_symbol(variable_name, variable_declaration.type->clone(), StaticAttribute { NoInit {}, true });
        }
    } else if (variable_declaration.storage_class == StorageClass::STATIC) {
        Initializer initial_value;
        if (!variable_declaration.expression.has_value()) {
            // conversion is performed at compile time
            auto con_res = convert_constant_type(ConstantType(0), *variable_declaration.type);
            if (!con_res.has_value()) {
                throw TypeCheckPassError(std::format("Failed convert_constant_type at:\n{}",
                    m_source_manager->get_source_line(variable_declaration.source_location)));
            }
            initial_value = InitialValue { con_res.value() };
        } else if (ConstantExpression* expr = dynamic_cast<ConstantExpression*>(variable_declaration.expression.value().get())) {
            // conversion is performed at compile time
            auto con_res = convert_constant_type(expr->value, *variable_declaration.type);
            if (!con_res.has_value()) {
                throw TypeCheckPassError(std::format("Failed convert_constant_type at:\n{}",
                    m_source_manager->get_source_line(variable_declaration.source_location)));
            }
            initial_value = InitialValue { con_res.value() };
        } else {
            throw TypeCheckPassError(std::format("Local variable declaration of {} has non-constant initializer!at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
        }
        m_symbol_table->insert_symbol(variable_name, variable_declaration.type->clone(), StaticAttribute { initial_value, false });
    } else { // local variable
        m_symbol_table->insert_symbol(variable_name, variable_declaration.type->clone(), LocalAttribute {});
        if (variable_declaration.expression.has_value()) {
            variable_declaration.expression.value()->accept(*this);
            convert_expression_to(variable_declaration.expression.value(), *variable_declaration.type);
        }
    }
}

std::unique_ptr<Type> TypeCheckPass::get_common_type(const Type& t1, const Type& t2)
{
    if (t1.equals(t2)) {
        return t1.clone();
    } else {
        return std::make_unique<LongType>();
    }
}

void TypeCheckPass::convert_expression_to(std::unique_ptr<Expression>& expr, const Type& target_type)
{
    if (expr->type->equals(target_type)) {
        return; // do nothing
    }
    std::unique_ptr<Expression> tmp = std::move(expr);
    // Wrap original expr into a CastExpr
    expr = std::make_unique<CastExpression>(tmp->source_location, target_type.clone(), std::move(tmp));
    expr->type = target_type.clone();
}

std::optional<InitialValueType> TypeCheckPass::convert_constant_type(const ConstantType& value, const Type& target_type)
{
    // Handle std::monostate input
    if (std::holds_alternative<std::monostate>(value)) {
        return std::nullopt;
    }

    if (dynamic_cast<const IntType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return std::get<int>(value);
        } else if (std::holds_alternative<long>(value)) {
            long long_val = std::get<long>(value);
            // Convert long to int using modulo 2^32 (assuming 32-bit int)
            // This truncates the value, keeping only the lower bits
            // TODO: add warning?
            return static_cast<int>(long_val);
        }
    } else if (dynamic_cast<const LongType*>(&target_type)) {
        if (std::holds_alternative<int>(value)) {
            return static_cast<long>(std::get<int>(value));
        } else if (std::holds_alternative<long>(value)) {
            return std::get<long>(value);
        }
    }

    return std::nullopt; // Unsupported target type
}
