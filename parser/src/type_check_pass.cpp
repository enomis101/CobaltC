#include "parser/type_check_pass.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "common/data/warning_manager.h"
#include "common/error/internal_compiler_error.h"
#include "parser/parser_ast.h"
#include <cmath>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <variant>

using namespace parser;

void TypeCheckPass::run()
{
    m_ast->accept(*this);
}

// ============================================================================
// Core Expression Type Checking Functions
// ============================================================================

void TypeCheckPass::typecheck_expression_and_convert(std::unique_ptr<Expression>& expr)
{
    // First typecheck the expression
    typecheck_expression(*expr);

    // Check if we need array-to-pointer conversion
    if (auto array_type = dynamic_cast<ArrayType*>(expr->type.get())) {
        // Create AddressOf expression for array-to-pointer decay
        auto addr_expr = std::make_unique<AddressOfExpression>(expr->source_location, std::move(expr));
        addr_expr->type = std::make_unique<PointerType>(array_type->element_type->clone());
        expr = std::move(addr_expr);
    }
}

void TypeCheckPass::typecheck_expression(Expression& expr)
{
    // Dispatch to appropriate typecheck method based on expression type
    if (auto* constant = dynamic_cast<ConstantExpression*>(&expr)) {
        typecheck_constant_expression(*constant);
    } else if (auto* variable = dynamic_cast<VariableExpression*>(&expr)) {
        typecheck_variable_expression(*variable);
    } else if (auto* unary = dynamic_cast<UnaryExpression*>(&expr)) {
        typecheck_unary_expression(*unary);
    } else if (auto* binary = dynamic_cast<BinaryExpression*>(&expr)) {
        typecheck_binary_expression(*binary);
    } else if (auto* assignment = dynamic_cast<AssignmentExpression*>(&expr)) {
        typecheck_assignment_expression(*assignment);
    } else if (auto* conditional = dynamic_cast<ConditionalExpression*>(&expr)) {
        typecheck_conditional_expression(*conditional);
    } else if (auto* function_call = dynamic_cast<FunctionCallExpression*>(&expr)) {
        typecheck_function_call_expression(*function_call);
    } else if (auto* cast = dynamic_cast<CastExpression*>(&expr)) {
        typecheck_cast_expression(*cast);
    } else if (auto* deref = dynamic_cast<DereferenceExpression*>(&expr)) {
        typecheck_dereference_expression(*deref);
    } else if (auto* addr_of = dynamic_cast<AddressOfExpression*>(&expr)) {
        typecheck_address_of_expression(*addr_of);
    } else if (auto* subscript = dynamic_cast<SubscriptExpression*>(&expr)) {
        typecheck_subscript_expression(*subscript);
    } else {
        throw InternalCompilerError("Unknown expression type in typecheck_expression");
    }
}

// ============================================================================
// Individual Expression Type Checking Methods
// ============================================================================

void TypeCheckPass::typecheck_constant_expression(ConstantExpression& node)
{
    if (std::holds_alternative<int>(node.value)) {
        node.type = std::make_unique<IntType>();
    } else if (std::holds_alternative<unsigned int>(node.value)) {
        node.type = std::make_unique<UnsignedIntType>();
    } else if (std::holds_alternative<long>(node.value)) {
        node.type = std::make_unique<LongType>();
    } else if (std::holds_alternative<unsigned long>(node.value)) {
        node.type = std::make_unique<UnsignedLongType>();
    } else if (std::holds_alternative<double>(node.value)) {
        node.type = std::make_unique<DoubleType>();
    } else {
        throw TypeCheckPassError(std::format("Unsupported ConstantExpression at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_variable_expression(VariableExpression& node)
{
    std::string& variable_name = node.identifier.name;
    auto& type = m_symbol_table->symbol_at(variable_name).type;
    if (dynamic_cast<FunctionType*>(type.get())) {
        throw TypeCheckPassError(std::format("Function name {} used as variable at:\n{}", variable_name, m_source_manager->get_source_line(node.source_location)));
    }

    node.type = type->clone();
}

void TypeCheckPass::typecheck_unary_expression(UnaryExpression& node)
{
    typecheck_expression_and_convert(node.expression);

    if (node.unary_operator == UnaryOperator::NOT) {
        // The results of expressions that evaluate to 1 or 0 to indicate true or false have type int.
        node.type = std::make_unique<IntType>();
    } else {
        node.type = node.expression->type->clone();
    }

    if (node.unary_operator == UnaryOperator::COMPLEMENT && is_type<DoubleType>(*node.expression->type)) {
        throw TypeCheckPassError(std::format("Bitwise complement operator does not accept double operands at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    if (node.unary_operator == UnaryOperator::NEGATE && is_type<PointerType>(*node.expression->type)) {
        throw TypeCheckPassError(std::format("Cannot apply negate operator to pointers at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    if (node.unary_operator == UnaryOperator::COMPLEMENT && is_type<PointerType>(*node.expression->type)) {
        throw TypeCheckPassError(std::format("Cannot apply complement operator to pointers at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_binary_expression(BinaryExpression& node)
{
    typecheck_expression_and_convert(node.left_expression);
    typecheck_expression_and_convert(node.right_expression);

    // logical operator evaluate to int
    if (node.binary_operator == BinaryOperator::AND || node.binary_operator == BinaryOperator::OR) {
        node.type = std::make_unique<IntType>();
        return;
    }

    auto left_type = node.left_expression->type->clone();
    auto right_type = node.right_expression->type->clone();

    // Handle special pointer type cases
    if (is_type<PointerType>(*left_type) || is_type<PointerType>(*right_type)) {
        switch (node.binary_operator) {
        case BinaryOperator::ADD: {
            if (is_type<PointerType>(*left_type) && right_type->is_integer()) {
                convert_expression_to<LongType>(node.right_expression);
                node.type = left_type->clone();
            } else if (is_type<PointerType>(*right_type) && left_type->is_integer()) {
                convert_expression_to<LongType>(node.left_expression);
                node.type = right_type->clone();
            } else {
                throw TypeCheckPassError(std::format("Invalid operands for pointer addition at:\n{}", m_source_manager->get_source_line(node.source_location)));
            }
            return;
        }
        case BinaryOperator::SUBTRACT: {
            if (is_type<PointerType>(*left_type) && right_type->is_integer()) {
                // you can subtract an integer from a pointer, but you can't subtract a pointer from an integer
                convert_expression_to<LongType>(node.right_expression);
                node.type = left_type->clone();
            } else if (is_type<PointerType>(*left_type) && left_type->equals(*right_type)) {
                // when subtracting two pointers both openrds must have the same type
                node.type = std::make_unique<LongType>();
            } else {
                throw TypeCheckPassError(std::format("Invalid operands for pointer subtraction at:\n{}", m_source_manager->get_source_line(node.source_location)));
            }
            return;
        }
        case BinaryOperator::GREATER_THAN:
        case BinaryOperator::GREATER_OR_EQUAL:
        case BinaryOperator::LESS_THAN:
        case BinaryOperator::LESS_OR_EQUAL: {
            // Pointer relational operators must have same type and return an int,
            if (!left_type->equals(*right_type)) {
                throw TypeCheckPassError(std::format("Invalid operands for pointer relational operator at:\n{}", m_source_manager->get_source_line(node.source_location)));
            }
            node.type = std::make_unique<IntType>();
            return;
        }
        case BinaryOperator::MULTIPLY:
            throw TypeCheckPassError(std::format("Multiply operator does not accept pointer operands at:\n{}", m_source_manager->get_source_line(node.source_location)));
        case BinaryOperator::DIVIDE:
            throw TypeCheckPassError(std::format("Divide operator does not accept pointer operands at:\n{}", m_source_manager->get_source_line(node.source_location)));
        case BinaryOperator::REMAINDER:
            throw TypeCheckPassError(std::format("Remainder operator does not accept pointer operands at:\n{}", m_source_manager->get_source_line(node.source_location)));
        default:
            break;
        }
    }

    // Get common type, handling pointer type
    auto common_type = get_common_pointer_type(*node.left_expression, *node.right_expression);

    convert_expression_to(node.left_expression, *common_type);
    convert_expression_to(node.right_expression, *common_type);

    switch (node.binary_operator) {
    case BinaryOperator::ADD:
    case BinaryOperator::SUBTRACT:
    case BinaryOperator::MULTIPLY:
    case BinaryOperator::DIVIDE:
    case BinaryOperator::REMAINDER:
        node.type = common_type->clone();
        break;
    default:
        node.type = std::make_unique<IntType>();
    }

    if (node.binary_operator == BinaryOperator::REMAINDER && is_type<DoubleType>(*node.type)) {
        throw TypeCheckPassError(std::format("Remainder operator does not accept double operands at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_assignment_expression(AssignmentExpression& node)
{
    // Process the left operand before checking if it's an value, to detect if we are tryin to assign to an array, as typecheck_expression_and_convert will wrap it in an AddressOfExpression
    typecheck_expression_and_convert(node.left_expression);
    if (!is_lvalue(*node.left_expression)) {
        throw TypeCheckPassError(std::format("In AssignmentExpression left expression is not an lvalue at:\n{}", m_source_manager->get_source_line(node.left_expression->source_location)));
    }

    typecheck_expression_and_convert(node.right_expression);

    auto left_type = node.left_expression->type->clone();
    if (!convert_expression_by_assignment(node.right_expression, *left_type)) {
        throw TypeCheckPassError(std::format("In AssignmentExpression cannot convert type for assignment at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
    node.type = std::move(left_type);
}

void TypeCheckPass::typecheck_conditional_expression(ConditionalExpression& node)
{
    typecheck_expression_and_convert(node.condition);
    typecheck_expression_and_convert(node.true_expression);
    typecheck_expression_and_convert(node.false_expression);

    auto true_type = node.true_expression->type->clone();
    auto false_type = node.false_expression->type->clone();

    // Find common type between two branches, handles pointer types
    auto common_type = get_common_type(*node.true_expression, *node.false_expression);

    // Convert both branches to the common type
    convert_expression_to(node.true_expression, *common_type);
    convert_expression_to(node.false_expression, *common_type);
    node.type = std::move(common_type);
}

void TypeCheckPass::typecheck_function_call_expression(FunctionCallExpression& node)
{
    std::string& function_name = node.name.name;
    auto& type = m_symbol_table->symbol_at(function_name).type;
    if (!dynamic_cast<FunctionType*>(type.get())) {
        throw TypeCheckPassError(std::format("Variable {} used as function name at:\n{}", function_name, m_source_manager->get_source_line(node.source_location)));
    }

    FunctionType* fun_type = dynamic_cast<FunctionType*>(type.get());
    if (fun_type->parameters_type.size() != node.arguments.size()) {
        throw TypeCheckPassError(std::format("Function {} called with the wrong number of arguments {} expected {} at:\n{}", function_name, node.arguments.size(), fun_type->parameters_type.size(), m_source_manager->get_source_line(node.source_location)));
    }

    // Visit arguments
    for (size_t i = 0; i < fun_type->parameters_type.size(); ++i) {
        auto& arg = node.arguments[i];
        auto& arg_type = fun_type->parameters_type[i];
        typecheck_expression_and_convert(arg);
        if (!convert_expression_by_assignment(arg, *arg_type)) {
            throw TypeCheckPassError(std::format("In function call cannot convert type for assignment at:\n{}", m_source_manager->get_source_line(node.source_location)));
        }
    }
    node.type = fun_type->return_type->clone();
}

void TypeCheckPass::typecheck_cast_expression(CastExpression& node)
{
    typecheck_expression_and_convert(node.expression);

    node.type = node.target_type->clone();

    if (is_type<PointerType>(*node.target_type) && is_type<DoubleType>(*node.expression->type)) {
        throw TypeCheckPassError(std::format("Cannot convert double to pointer at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    if (is_type<DoubleType>(*node.target_type) && is_type<PointerType>(*node.expression->type)) {
        throw TypeCheckPassError(std::format("Cannot convert pointer to double at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    if (is_type<ArrayType>(*node.target_type)) {
        throw TypeCheckPassError(std::format("Cannot cast to array at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_dereference_expression(DereferenceExpression& node)
{
    typecheck_expression_and_convert(node.expression);

    if (auto ptr_type = dynamic_cast<PointerType*>(node.expression->type.get())) {
        node.type = ptr_type->referenced_type->clone();
    } else {
        throw TypeCheckPassError(std::format("Cannot deference non-pointer type at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_address_of_expression(AddressOfExpression& node)
{
    if (is_lvalue(*node.expression)) {
        typecheck_expression_and_convert(node.expression);
        node.type = std::make_unique<PointerType>(node.expression->type->clone());
    } else {
        throw TypeCheckPassError(std::format("Can't take the address of a non-lvalue at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }
}

void TypeCheckPass::typecheck_subscript_expression(SubscriptExpression& node)
{
    typecheck_expression_and_convert(node.expression1);
    typecheck_expression_and_convert(node.expression2);

    const auto& t1 = node.expression1->type;
    const auto& t2 = node.expression2->type;
    const auto& ptr_type_ref = is_type<PointerType>(*t1) ? t1 : t2;
    if (is_type<PointerType>(*t1) && t2->is_integer()) {
        convert_expression_to<LongType>(node.expression2);
    } else if (t1->is_integer() && is_type<PointerType>(*t2)) {
        convert_expression_to<LongType>(node.expression1);
    } else {
        throw TypeCheckPassError(std::format("Invalid operands for SubscriptExpression at:\n{}", m_source_manager->get_source_line(node.source_location)));
    }

    auto ptr_type = dynamic_cast<PointerType*>(ptr_type_ref.get());
    if (!ptr_type) {
        throw InternalCompilerError("ptr_type must be valid");
    }

    node.type = ptr_type->referenced_type->clone();
}

void TypeCheckPass::typecheck_initializer(const Type& target_type, Initializer& init)
{
    if (auto single_init = dynamic_cast<SingleInitializer*>(&init)) {
        typecheck_expression_and_convert(single_init->expression);
        if (!convert_expression_by_assignment(single_init->expression, target_type)) {
            throw TypeCheckPassError(std::format("In typecheck_initializer cannot convert type for assignment at:\n{}", m_source_manager->get_source_line(init.source_location)));
        }
        single_init->type = target_type.clone();
    } else if (auto compound_init = dynamic_cast<CompoundInitializer*>(&init)) {
        if (auto arr_type = dynamic_cast<const ArrayType*>(&target_type)) {
            if (compound_init->initializer_list.size() > arr_type->size) {
                throw TypeCheckPassError(std::format("Too many initializers at:\n{}", m_source_manager->get_source_line(init.source_location)));
            }
            for (auto& inner_init : compound_init->initializer_list) {
                typecheck_initializer(*arr_type->element_type, *inner_init);
            }
            // pad with zeros
            for (size_t i = compound_init->initializer_list.size(); i < arr_type->size; ++i) {
                compound_init->initializer_list.emplace_back(get_zero_initializer(init.source_location, *arr_type->element_type));
            }
        } else {
            throw TypeCheckPassError(std::format("Can't initialize scalar object with a compound initializer at:\n{}", m_source_manager->get_source_line(init.source_location)));
        }
    } else {
        throw InternalCompilerError("Unsupported type in typecheck_initializer");
    }
}

std::unique_ptr<Initializer> TypeCheckPass::get_zero_initializer(SourceLocationIndex loc, const Type& type)
{
    if (auto arr_type = dynamic_cast<const ArrayType*>(&type)) {
        std::vector<std::unique_ptr<Initializer>> initializer_list;
        for (size_t i = 0; i < arr_type->size; ++i) {
            initializer_list.emplace_back(get_zero_initializer(loc, *arr_type->element_type));
        }
        return std::make_unique<CompoundInitializer>(loc, std::move(initializer_list));
    } else if (type.is_scalar()) {
        auto res = SymbolTable::convert_constant_type(0, type);
        if (!res.has_value()) {
            throw InternalCompilerError("Something went wrong with convert_constant_type in get_zero_initializer");
        }
        auto const_expr = std::make_unique<ConstantExpression>(loc, res.value());
        return std::make_unique<SingleInitializer>(loc, std::move(const_expr));
    } else {
        throw InternalCompilerError("Unsupported type in get_zero_initializer");
    }
}

StaticInitialValue TypeCheckPass::convert_static_initializer(const Type& target_type, Initializer& init, std::function<void(const std::string&)> warning_callback)
{
    if (auto single_init = dynamic_cast<SingleInitializer*>(&init)) {
        typecheck_expression_and_convert(single_init->expression);

        auto const_expr = dynamic_cast<ConstantExpression*>(single_init->expression.get());
        if (!const_expr) {
            throw TypeCheckPassError(std::format("Static variable declaration has non-constant initializer! at:\n{}",
                m_source_manager->get_source_line(init.source_location)));
        }
        single_init->type = target_type.clone();
        return convert_constant_type_by_assignment(const_expr->value, target_type, init.source_location, warning_callback);
    } else if (auto compound_init = dynamic_cast<CompoundInitializer*>(&init)) {
        if (auto arr_type = dynamic_cast<const ArrayType*>(&target_type)) {
            if (compound_init->initializer_list.size() > arr_type->size) {
                throw TypeCheckPassError(std::format("Too many initializers at:\n{}", m_source_manager->get_source_line(init.source_location)));
            }
            std::vector<StaticInitialValueType> initial_values;
            for (auto& inner_init : compound_init->initializer_list) {
                for (auto& elem : convert_static_initializer(*arr_type->element_type, *inner_init, warning_callback).values) {
                    initial_values.push_back(elem);
                }
            }

            size_t size_diff = arr_type->size - compound_init->initializer_list.size();
            // pad with zeros
            if (size_diff > 0) {
                auto zero_init = ZeroInit { get_static_zero_initializer(*arr_type->element_type) * size_diff };
                initial_values.push_back(StaticInitialValueType(zero_init));
            }

            // merge adjacents zero inits
            StaticInitialValue res;
            for (auto& val : initial_values) {
                if (val.is_zero() && res.values.size() > 0 && res.values.back().is_zero()) {
                    res.values.back().set_zero_size(res.values.back().zero_size() + val.zero_size());
                } else {
                    res.values.push_back(val);
                }
            }
            return res;
        } else {
            throw TypeCheckPassError(std::format("Can't initialize scalar object with a compound initializer at:\n{}", m_source_manager->get_source_line(init.source_location)));
        }
    } else {
        throw InternalCompilerError("Unsupported type in typecheck_initializer");
    }
}

size_t TypeCheckPass::get_static_zero_initializer(const Type& type)
{
    if (auto arr_type = dynamic_cast<const ArrayType*>(&type)) {
        return get_static_zero_initializer(*arr_type->element_type) * arr_type->size;
    } else if (type.is_scalar()) {
        return type.size();
    } else {
        throw InternalCompilerError("Unsupported type in get_static_zero_initializer");
    }
}

// ============================================================================
// Visitor Methods for Non-Expression Nodes
// ============================================================================

void TypeCheckPass::visit(Identifier& node)
{
    // Empty method
}

void TypeCheckPass::visit(ReturnStatement& node)
{
    typecheck_expression_and_convert(node.expression);

    if (!m_current_function_declaration) {
        throw InternalCompilerError("m_current_function_declaration must be valid!");
    }

    const auto& function_name = m_current_function_declaration->name.name;
    auto& type = m_symbol_table->symbol_at(function_name).type;
    if (FunctionType* fun_type = dynamic_cast<FunctionType*>(type.get())) {
        if (!convert_expression_by_assignment(node.expression, *fun_type->return_type)) {
            throw TypeCheckPassError(std::format("In return statement cannot convert type for assignment at:\n{}", m_source_manager->get_source_line(node.source_location)));
        }
    } else {
        throw InternalCompilerError("m_current_function_declaration is not a function pointer");
    }
}

void TypeCheckPass::visit(FunctionDeclaration& function_declaration)
{
    const auto& function_type = dynamic_cast<FunctionType*>(function_declaration.type.get());
    const std::string& function_name = function_declaration.name.name;
    if (is_type<ArrayType>(*function_type->return_type)) {
        throw TypeCheckPassError(std::format("Function {} cant return an array at:\n{}", function_name, m_source_manager->get_source_line(function_declaration.source_location)));
    }

    for (auto& param : function_type->parameters_type) {
        if (auto arr_type = dynamic_cast<ArrayType*>(param.get())) {
            auto ptr_type = std::make_unique<PointerType>(arr_type->element_type->clone());
            param = std::move(ptr_type);
        }
    }

    bool has_body = function_declaration.body.has_value();
    bool already_defined = false;

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

    if (function_declaration.params.size() != function_type->parameters_type.size()) {
        throw InternalCompilerError("function_declaration.params.size() must be equal to function_type->parameters_type.size()");
    }

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

void TypeCheckPass::visit(Program& node)
{
    for (auto& decl : node.declarations) {
        decl->accept(*this);
    }
}

void TypeCheckPass::visit(ExpressionStatement& node)
{
    typecheck_expression_and_convert(node.expression);
}

void TypeCheckPass::visit(IfStatement& node)
{
    typecheck_expression_and_convert(node.condition);
    node.then_statement->accept(*this);

    if (node.else_statement.has_value()) {
        node.else_statement.value()->accept(*this);
    }
}

void TypeCheckPass::visit(NullStatement& node)
{
    // Empty method
}

void TypeCheckPass::visit(VariableDeclaration& node)
{
    if (node.scope == DeclarationScope::File) {
        typecheck_file_scope_variable_declaration(node);
    } else {
        typecheck_local_variable_declaration(node);
    }
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
    typecheck_expression_and_convert(node.condition);
    node.body->accept(*this);
}

void TypeCheckPass::visit(DoWhileStatement& node)
{
    typecheck_expression_and_convert(node.condition);
    node.body->accept(*this);
}

void TypeCheckPass::visit(ForStatement& node)
{
    node.init->accept(*this);

    if (node.condition.has_value()) {
        typecheck_expression_and_convert(node.condition.value());
    }

    if (node.post.has_value()) {
        typecheck_expression_and_convert(node.post.value());
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
        typecheck_expression_and_convert(node.expression.value());
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

void TypeCheckPass::typecheck_file_scope_variable_declaration(VariableDeclaration& variable_declaration)
{
    const std::string& variable_name = variable_declaration.identifier.name;
    StaticInitializer initial_value;
    if (!variable_declaration.expression.has_value()) {
        if (variable_declaration.storage_class == StorageClass::EXTERN) {
            initial_value = NoInit {};
        } else {
            initial_value = TentativeInit {};
        }
    } else {
        // conversion is performed at compile time
        std::function<void(const std::string&)> warning_callback = [&](const std::string& message) { m_warning_manager->raise_warning(ParserWarningType::CAST, std::format("typecheck_file_scope_variable_declaration {} at:\n", message, m_source_manager->get_source_line(variable_declaration.source_location))); };

        initial_value = convert_static_initializer(*variable_declaration.type, *variable_declaration.expression.value(), warning_callback);
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
        if (std::holds_alternative<StaticInitialValue>(old_attr.init)) {
            if (std::holds_alternative<StaticInitialValue>(initial_value)) {
                throw TypeCheckPassError(std::format("Conflicting file scope variable definitions for {} at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
            } else {
                initial_value = old_attr.init;
            }
        } else if (!std::holds_alternative<StaticInitialValue>(initial_value) && std::holds_alternative<TentativeInit>(old_attr.init)) {
            initial_value = TentativeInit {};
        }
    }
    StaticAttribute attr(initial_value, global);
    m_symbol_table->insert_or_assign_symbol(variable_name, variable_declaration.type->clone(), attr);
}

void TypeCheckPass::typecheck_local_variable_declaration(VariableDeclaration& variable_declaration)
{
    const std::string& variable_name = variable_declaration.identifier.name;
    std::function<void(const std::string&)> warning_callback = [&](const std::string& message) {
        m_warning_manager->raise_warning(ParserWarningType::CAST,
            std::format("typecheck_local_variable_declaration {} at:\n", message, m_source_manager->get_source_line(variable_declaration.source_location)));
    };

    if (variable_declaration.storage_class == StorageClass::EXTERN) {
        if (variable_declaration.expression.has_value()) {
            throw TypeCheckPassError(std::format("StaticInitializer on local extern variable declaration for {} at:\n{}", variable_name, m_source_manager->get_source_line(variable_declaration.source_location)));
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
        StaticInitializer initial_value;
        if (!variable_declaration.expression.has_value()) {
            // conversion is performed at compile time
            initial_value = convert_constant_type_by_assignment(ConstantType(0), *variable_declaration.type, variable_declaration.source_location, warning_callback);
        } else {
            // conversion is performed at compile time
            initial_value = convert_static_initializer(*variable_declaration.type, *variable_declaration.expression.value(), warning_callback);
        }

        m_symbol_table->insert_symbol(variable_name, variable_declaration.type->clone(), StaticAttribute { initial_value, false });
    } else { // local variable
        m_symbol_table->insert_symbol(variable_name, variable_declaration.type->clone(), LocalAttribute {});
        if (variable_declaration.expression.has_value()) {
            typecheck_initializer(*variable_declaration.type, *variable_declaration.expression.value());
        }
    }
}

std::unique_ptr<Type> TypeCheckPass::get_common_type(const Type& t1, const Type& t2)
{
    if (t1.equals(t2)) {
        return t1.clone();
    } else if (is_type<DoubleType>(t1) || is_type<DoubleType>(t2)) {
        return std::make_unique<DoubleType>();
    } else if (t1.size() == t2.size()) {
        if (t1.is_signed()) {
            return t2.clone();
        } else {
            return t1.clone();
        }
    } else if (t1.size() > t2.size()) {
        return t1.clone();
    } else {
        return t2.clone();
    }
}

std::unique_ptr<Type> TypeCheckPass::get_common_type(const Expression& expr1, const Expression& expr2)
{
    if (is_type<PointerType>(*expr1.type) || is_type<PointerType>(*expr2.type)) {
        return get_common_pointer_type(expr1, expr2);
    } else {
        return get_common_type(*expr1.type, *expr2.type);
    }
}

std::unique_ptr<Type> TypeCheckPass::get_common_pointer_type(const Expression& expr1, const Expression& expr2)
{
    const Type& t1 = *expr1.type;
    const Type& t2 = *expr2.type;
    if (t1.equals(t2)) {
        return t1.clone();
    } else if (is_null_pointer_constant_expression(expr1)) {
        return t2.clone();
    } else if (is_null_pointer_constant_expression(expr2)) {
        return t1.clone();
    }

    throw InternalCompilerError("If they are pointer type get_common_pointer_type should always return a value");
}

bool TypeCheckPass::is_null_pointer_constant_expression(const Expression& expr)
{
    if (auto constant = dynamic_cast<const ConstantExpression*>(&expr)) {
        return SymbolTable::is_null_pointer_constant(constant->value);
    }
    return false;
}

bool TypeCheckPass::convert_expression_by_assignment(std::unique_ptr<Expression>& expr, const Type& target_type)
{
    const Type& expr_type = *expr->type;
    if (expr_type.equals(target_type)) {
        return true;
    }

    if (expr_type.is_arithmetic() && target_type.is_arithmetic()) {
        convert_expression_to(expr, target_type);
        return true;
    } else if (is_null_pointer_constant_expression(*expr) && is_type<PointerType>(target_type)) {
        convert_expression_to(expr, target_type);
        return true;
    }

    return false;
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

StaticInitialValue TypeCheckPass::convert_constant_type_by_assignment(const ConstantType& value, const Type& target_type, SourceLocationIndex loc, std::function<void(const std::string&)> warning_callback)
{
    StaticInitialValue static_init;
    auto res = SymbolTable::convert_constant_type(value, target_type, warning_callback);
    if (!res) {
        throw TypeCheckPassError(std::format("Failed convert_constant_type {}  at:\n{}",
            res.error(), m_source_manager->get_source_line(loc)));
    }

    static_init.values = { StaticInitialValueType(res.value()) };
    return static_init;
}

bool TypeCheckPass::is_lvalue(const Expression& expr)
{
    return dynamic_cast<const VariableExpression*>(&expr) || dynamic_cast<const DereferenceExpression*>(&expr) || dynamic_cast<const SubscriptExpression*>(&expr);
}
