#include "tacky/tacky_generator.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "parser/parser_ast.h"
#include "tacky/tacky_ast.h"
#include <cassert>
#include <memory>
#include <string>
#include <unordered_map>

using namespace tacky;

TackyGenerator::TackyGenerator(std::shared_ptr<parser::ParserAST> ast, std::shared_ptr<NameGenerator> name_generator, std::shared_ptr<SymbolTable> symbol_table)
    : m_ast { ast }
    , m_name_generator { name_generator }
    , m_symbol_table { symbol_table }
{
    if (!m_ast || !dynamic_cast<parser::Program*>(m_ast.get())) {
        throw TackyGeneratorError("TackyGenerator: Invalid AST");
    }
}

std::shared_ptr<TackyAST> TackyGenerator::generate()
{
    std::shared_ptr<TackyAST> program = transform_program(*dynamic_cast<parser::Program*>(m_ast.get()));
    transform_symbols_to_tacky(program);
    return program;
}

void TackyGenerator::transform_symbols_to_tacky(std::shared_ptr<TackyAST> tacky_ast)
{
    auto& top_levels = dynamic_cast<Program*>(tacky_ast.get())->definitions;

    for (const auto& p : m_symbol_table->symbols()) {
        const auto& entry = p.second;
        if (std::holds_alternative<StaticAttribute>(entry.attribute)) {
            const auto& static_attr = std::get<StaticAttribute>(entry.attribute);
            bool global = static_attr.global;
            const std::string& variable_name = p.first;
            if (std::holds_alternative<StaticInitialValue>(static_attr.init)) {

                auto initial_value = SymbolTable::convert_constant_type(std::get<StaticInitialValue>(static_attr.init).value, *entry.type);
                assert(initial_value.has_value());
                top_levels.emplace_back(std::make_unique<StaticVariable>(variable_name, global, entry.type->clone(), initial_value.value()));
            } else if (std::holds_alternative<TentativeInit>(static_attr.init)) {
                auto initial_value = SymbolTable::convert_constant_type(0, *entry.type);
                assert(initial_value.has_value());
                top_levels.emplace_back(std::make_unique<StaticVariable>(variable_name, global, entry.type->clone(), initial_value.value()));
            } else if (std::holds_alternative<NoInit>(static_attr.init)) {
                continue;
            }
        }
    }
}

UnaryOperator TackyGenerator::transform_unary_operator(parser::UnaryOperator& unary_operator)
{
    static const std::unordered_map<parser::UnaryOperator, UnaryOperator> unary_op_map = {
        { parser::UnaryOperator::NEGATE, UnaryOperator::NEGATE },
        { parser::UnaryOperator::COMPLEMENT, UnaryOperator::COMPLEMENT },
        { parser::UnaryOperator::NOT, UnaryOperator::NOT }
    };

    auto it = unary_op_map.find(unary_operator);
    if (it != unary_op_map.end()) {
        return it->second;
    }

    throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported UnaryOperator");
}

BinaryOperator TackyGenerator::transform_binary_operator(parser::BinaryOperator& binary_operator)
{
    static const std::unordered_map<parser::BinaryOperator, BinaryOperator> binary_op_map = {
        { parser::BinaryOperator::MULTIPLY, BinaryOperator::MULTIPLY },
        { parser::BinaryOperator::DIVIDE, BinaryOperator::DIVIDE },
        { parser::BinaryOperator::REMAINDER, BinaryOperator::REMAINDER },
        { parser::BinaryOperator::ADD, BinaryOperator::ADD },
        { parser::BinaryOperator::SUBTRACT, BinaryOperator::SUBTRACT },
        { parser::BinaryOperator::EQUAL, BinaryOperator::EQUAL },
        { parser::BinaryOperator::NOT_EQUAL, BinaryOperator::NOT_EQUAL },
        { parser::BinaryOperator::LESS_THAN, BinaryOperator::LESS_THAN },
        { parser::BinaryOperator::LESS_OR_EQUAL, BinaryOperator::LESS_OR_EQUAL },
        { parser::BinaryOperator::GREATER_THAN, BinaryOperator::GREATER_THAN },
        { parser::BinaryOperator::GREATER_OR_EQUAL, BinaryOperator::GREATER_OR_EQUAL }
    };

    auto it = binary_op_map.find(binary_operator);
    if (it != binary_op_map.end()) {
        return it->second;
    }

    throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported BinaryOperator");
}

std::unique_ptr<Value> TackyGenerator::transform_expression(parser::Expression& expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ConstantExpression* constant_expression = dynamic_cast<parser::ConstantExpression*>(&expression)) {
        return transform_constant_expression(*constant_expression);
    } else if (parser::UnaryExpression* unary_expression = dynamic_cast<parser::UnaryExpression*>(&expression)) {
        return transform_unary_expression(*unary_expression, instructions);
    } else if (parser::BinaryExpression* binary_expression = dynamic_cast<parser::BinaryExpression*>(&expression)) {
        return transform_binary_expression(*binary_expression, instructions);
    } else if (parser::VariableExpression* variable_expression = dynamic_cast<parser::VariableExpression*>(&expression)) {
        return transform_variable_expression(*variable_expression);
    } else if (parser::AssignmentExpression* assignment_expression = dynamic_cast<parser::AssignmentExpression*>(&expression)) {
        return transform_assignment_expression(*assignment_expression, instructions);
    } else if (parser::ConditionalExpression* conditional_expression = dynamic_cast<parser::ConditionalExpression*>(&expression)) {
        return transform_conditional_expression(*conditional_expression, instructions);
    } else if (parser::FunctionCallExpression* function_call_expression = dynamic_cast<parser::FunctionCallExpression*>(&expression)) {
        return transform_function_call_expression(*function_call_expression, instructions);
    } else if (parser::CastExpression* cast_expression = dynamic_cast<parser::CastExpression*>(&expression)) {
        return transform_cast_expression(*cast_expression, instructions);
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported Expression");
    }
}

std::unique_ptr<Value> TackyGenerator::transform_constant_expression(parser::ConstantExpression& constant_expression)
{
    return std::make_unique<Constant>(constant_expression.value);
}

std::unique_ptr<Value> TackyGenerator::transform_unary_expression(parser::UnaryExpression& unary_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> src = transform_expression(*unary_expression.expression, instructions);
    std::string dst_name = make_and_add_temporary(*unary_expression.type);
    std::unique_ptr<TemporaryVariable> dst = std::make_unique<TemporaryVariable>(dst_name);
    std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(*dst);
    UnaryOperator op = transform_unary_operator(unary_expression.unary_operator);
    instructions.emplace_back(std::make_unique<UnaryInstruction>(op, std::move(src), std::move(dst)));
    return dst_copy;
}

std::unique_ptr<Value> TackyGenerator::transform_binary_expression(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    // We need to handle logical AND OR differently to support short circuit evaluation
    if (binary_expression.binary_operator == parser::BinaryOperator::AND) {
        return transform_logical_and(binary_expression, instructions);
    } else if (binary_expression.binary_operator == parser::BinaryOperator::OR) {
        return transform_logical_or(binary_expression, instructions);
    } else {
        std::unique_ptr<Value> src1 = transform_expression(*binary_expression.left_expression, instructions);
        std::unique_ptr<Value> src2 = transform_expression(*binary_expression.right_expression, instructions);
        std::string dst_name = make_and_add_temporary(*binary_expression.type);
        std::unique_ptr<TemporaryVariable> dst = std::make_unique<TemporaryVariable>(dst_name);
        std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(*dst);
        BinaryOperator op = transform_binary_operator(binary_expression.binary_operator);
        instructions.emplace_back(std::make_unique<BinaryInstruction>(op, std::move(src1), std::move(src2), std::move(dst)));
        return dst_copy;
    }
}

std::unique_ptr<Value> TackyGenerator::transform_logical_and(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> src1 = transform_expression(*binary_expression.left_expression, instructions);
    std::string false_label = m_name_generator->make_label("and_false");
    instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(src1), false_label));

    std::unique_ptr<Value> src2 = transform_expression(*binary_expression.right_expression, instructions);
    instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(src2), false_label));

    std::string result = make_and_add_temporary(*binary_expression.type);

    // Set result to 1 (true)
    {
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::make_unique<Constant>(1), std::move(result_var)));
    }

    std::string end_label = m_name_generator->make_label("and_end");
    instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
    instructions.emplace_back(std::make_unique<LabelInstruction>(false_label));

    // Set result to 0 (false)
    {
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::make_unique<Constant>(0), std::move(result_var)));
    }

    instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
    return std::make_unique<TemporaryVariable>(result);
}

std::unique_ptr<Value> TackyGenerator::transform_logical_or(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> src1 = transform_expression(*binary_expression.left_expression, instructions);
    std::string true_label = m_name_generator->make_label("or_true");
    instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(src1), true_label));

    std::unique_ptr<Value> src2 = transform_expression(*binary_expression.right_expression, instructions);
    instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(src2), true_label));

    std::string result = make_and_add_temporary(*binary_expression.type);

    // Set result to 0 (false)
    {
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::make_unique<Constant>(0), std::move(result_var)));
    }

    std::string end_label = m_name_generator->make_label("or_end");
    instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
    instructions.emplace_back(std::make_unique<LabelInstruction>(true_label));

    // Set result to 1 (true)
    {
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::make_unique<Constant>(1), std::move(result_var)));
    }

    instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
    return std::make_unique<TemporaryVariable>(result);
}

std::unique_ptr<Value> TackyGenerator::transform_variable_expression(parser::VariableExpression& variable_expression)
{
    return std::make_unique<TemporaryVariable>(variable_expression.identifier.name);
}

std::unique_ptr<Value> TackyGenerator::transform_assignment_expression(parser::AssignmentExpression& assignment_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    parser::VariableExpression* variable = dynamic_cast<parser::VariableExpression*>(assignment_expression.left_expression.get());
    if (!variable) {
        throw TackyGeneratorError("TackyGenerator: Left side of AssignmentExpression is not a variable something went wrong with Semantic Analysis stage");
    }
    std::unique_ptr<Value> right = transform_expression(*assignment_expression.right_expression, instructions);
    instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(right), std::make_unique<TemporaryVariable>(variable->identifier.name)));
    return std::make_unique<TemporaryVariable>(variable->identifier.name);
}

std::unique_ptr<Value> TackyGenerator::transform_conditional_expression(parser::ConditionalExpression& conditional_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    // Create labels
    std::string false_label = m_name_generator->make_label("conditional_false");
    std::string end_label = m_name_generator->make_label("conditional_end");
    std::string result = make_and_add_temporary(*conditional_expression.type);

    // Evaluate condition
    std::unique_ptr<Value> cond = transform_expression(*conditional_expression.condition, instructions);
    instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), false_label));

    // True branch
    std::unique_ptr<Value> true_value = transform_expression(*conditional_expression.true_expression, instructions);
    {
        // result = true_expression
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(true_value), std::move(result_var)));
    }
    instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));

    // False branch
    instructions.emplace_back(std::make_unique<LabelInstruction>(false_label));
    std::unique_ptr<Value> false_value = transform_expression(*conditional_expression.false_expression, instructions);
    {
        // result = false_expression
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(false_value), std::move(result_var)));
    }

    instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
    return std::make_unique<TemporaryVariable>(result);
}

std::unique_ptr<Value> TackyGenerator::transform_function_call_expression(parser::FunctionCallExpression& function_call_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::vector<std::unique_ptr<Value>> args;
    for (auto& arg : function_call_expression.arguments) {
        args.emplace_back(transform_expression((*arg.get()), instructions));
    }
    std::string result = make_and_add_temporary(*function_call_expression.type);
    std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
    instructions.emplace_back(std::make_unique<FunctionCallInstruction>(function_call_expression.name.name, std::move(args), std::move(result_var)));
    return std::make_unique<TemporaryVariable>(result);
}

std::unique_ptr<Value> TackyGenerator::transform_cast_expression(parser::CastExpression& cast_expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> expr_res = transform_expression(*cast_expression.expression, instructions);

    // If the types are the same, no cast is needed
    if (cast_expression.expression->type->equals(*cast_expression.target_type)) {
        return expr_res;
    }

    std::string tmp_name = make_and_add_temporary(*cast_expression.target_type, LocalAttribute {});
    std::unique_ptr<Value> dst = std::make_unique<TemporaryVariable>(tmp_name);
    std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(tmp_name);

    if (cast_expression.target_type->equals(LongType {})) {
        instructions.emplace_back(std::make_unique<SignExtendInstruction>(std::move(expr_res), std::move(dst)));
    } else {
        instructions.emplace_back(std::make_unique<TruncateInstruction>(std::move(expr_res), std::move(dst)));
    }
    return dst_copy;
}

void TackyGenerator::transform_statement(parser::Statement& statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ReturnStatement* return_statement = dynamic_cast<parser::ReturnStatement*>(&statement)) {
        transform_return_statement(*return_statement, instructions);
    } else if (parser::ExpressionStatement* expression_statement = dynamic_cast<parser::ExpressionStatement*>(&statement)) {
        transform_expression_statement(*expression_statement, instructions);
    } else if (parser::IfStatement* if_statement = dynamic_cast<parser::IfStatement*>(&statement)) {
        transform_if_statement(*if_statement, instructions);
    } else if (parser::CompoundStatement* compound_statement = dynamic_cast<parser::CompoundStatement*>(&statement)) {
        transform_compound_statement(*compound_statement, instructions);
    } else if (parser::BreakStatement* break_statement = dynamic_cast<parser::BreakStatement*>(&statement)) {
        transform_break_statement(*break_statement, instructions);
    } else if (parser::ContinueStatement* continue_statement = dynamic_cast<parser::ContinueStatement*>(&statement)) {
        transform_continue_statement(*continue_statement, instructions);
    } else if (parser::DoWhileStatement* do_while_statement = dynamic_cast<parser::DoWhileStatement*>(&statement)) {
        transform_do_while_statement(*do_while_statement, instructions);
    } else if (parser::WhileStatement* while_statement = dynamic_cast<parser::WhileStatement*>(&statement)) {
        transform_while_statement(*while_statement, instructions);
    } else if (parser::ForStatement* for_statement = dynamic_cast<parser::ForStatement*>(&statement)) {
        transform_for_statement(*for_statement, instructions);
    } else if (parser::NullStatement* null_statement = dynamic_cast<parser::NullStatement*>(&statement)) {
        transform_null_statement(*null_statement, instructions);
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported Statement");
    }
}

void TackyGenerator::transform_return_statement(parser::ReturnStatement& return_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> value = transform_expression(*(return_statement.expression.get()), instructions);
    instructions.emplace_back(std::make_unique<ReturnInstruction>(std::move(value)));
}

void TackyGenerator::transform_expression_statement(parser::ExpressionStatement& expression_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> value = transform_expression(*(expression_statement.expression.get()), instructions);
    // value is discarded
}

void TackyGenerator::transform_if_statement(parser::IfStatement& if_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::unique_ptr<Value> cond = transform_expression(*(if_statement.condition.get()), instructions);

    if (!if_statement.else_statement.has_value()) {
        // if without else
        std::string end_label = m_name_generator->make_label("if_end");
        instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), end_label));
        transform_statement(*if_statement.then_statement, instructions);
        instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
    } else {
        // if with else
        std::string else_label = m_name_generator->make_label("else");
        std::string end_label = m_name_generator->make_label("if_end");
        instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), else_label));
        transform_statement(*if_statement.then_statement, instructions);
        instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
        instructions.emplace_back(std::make_unique<LabelInstruction>(else_label));
        transform_statement(*if_statement.else_statement.value(), instructions);
        instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
    }
}

void TackyGenerator::transform_compound_statement(parser::CompoundStatement& compound_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    transform_block(*compound_statement.block.get(), instructions);
}

void TackyGenerator::transform_break_statement(parser::BreakStatement& break_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::string break_label = "break_" + break_statement.label.name;
    instructions.emplace_back(std::make_unique<JumpInstruction>(break_label));
}

void TackyGenerator::transform_continue_statement(parser::ContinueStatement& continue_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::string continue_label = "continue_" + continue_statement.label.name;
    instructions.emplace_back(std::make_unique<JumpInstruction>(continue_label));
}

void TackyGenerator::transform_do_while_statement(parser::DoWhileStatement& do_while_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::string start_label = m_name_generator->make_label("do_while_start");
    std::string continue_label = "continue_" + do_while_statement.label.name;
    std::string break_label = "break_" + do_while_statement.label.name;

    instructions.emplace_back(std::make_unique<LabelInstruction>(start_label));
    transform_statement(*do_while_statement.body, instructions);
    instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));
    std::unique_ptr<Value> cond = transform_expression(*(do_while_statement.condition.get()), instructions);
    instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(cond), start_label));
    instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
}

void TackyGenerator::transform_while_statement(parser::WhileStatement& while_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::string continue_label = "continue_" + while_statement.label.name;
    std::string break_label = "break_" + while_statement.label.name;

    instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));
    std::unique_ptr<Value> cond = transform_expression(*(while_statement.condition.get()), instructions);
    instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), break_label));

    transform_statement(*while_statement.body, instructions);

    instructions.emplace_back(std::make_unique<JumpInstruction>(continue_label));
    instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
}

void TackyGenerator::transform_for_statement(parser::ForStatement& for_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    std::string start_label = m_name_generator->make_label("for_start");
    std::string continue_label = "continue_" + for_statement.label.name;
    std::string break_label = "break_" + for_statement.label.name;

    // Initialize
    transform_for_init(*for_statement.init, instructions);

    instructions.emplace_back(std::make_unique<LabelInstruction>(start_label));

    // Condition
    if (for_statement.condition.has_value()) {
        std::unique_ptr<Value> cond = transform_expression(*(for_statement.condition.value().get()), instructions);
        instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), break_label));
    }

    // Body
    transform_statement(*for_statement.body, instructions);

    // Continue label (where post expression is evaluated)
    instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));

    // Post expression
    if (for_statement.post.has_value()) {
        transform_expression(*(for_statement.post.value().get()), instructions);
    }

    instructions.emplace_back(std::make_unique<JumpInstruction>(start_label));
    instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
}

void TackyGenerator::transform_null_statement(parser::NullStatement& null_statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    // do nothing
}

void TackyGenerator::transform_declaration(parser::Declaration& declaration, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::VariableDeclaration* variable_declaration = dynamic_cast<parser::VariableDeclaration*>(&declaration)) {
        // We do not generate any code for local variable declarations with static or external specifiers
        if (variable_declaration->storage_class == parser::StorageClass::NONE && variable_declaration->expression.has_value()) {
            std::unique_ptr<Value> value = transform_expression(*variable_declaration->expression.value(), instructions);
            instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(value), std::make_unique<TemporaryVariable>(variable_declaration->identifier.name)));
        }
    } else if (dynamic_cast<parser::FunctionDeclaration*>(&declaration)) {
        // DO NOTHING
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported Declaration");
    }
}

void TackyGenerator::transform_for_init(parser::ForInit& for_init, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ForInitDeclaration* declaration = dynamic_cast<parser::ForInitDeclaration*>(&for_init)) {
        transform_declaration(*declaration->declaration, instructions);
    } else if (parser::ForInitExpression* expr = dynamic_cast<parser::ForInitExpression*>(&for_init)) {
        if (expr->expression.has_value()) {
            transform_expression(*expr->expression.value(), instructions);
        }
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported ForInit");
    }
}

void TackyGenerator::transform_block_item(parser::BlockItem& block_item, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::Declaration* declaration = dynamic_cast<parser::Declaration*>(&block_item)) {
        transform_declaration(*declaration, instructions);
    } else if (parser::Statement* statement = dynamic_cast<parser::Statement*>(&block_item)) {
        transform_statement(*statement, instructions);
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsupported BlockItem");
    }
}

void TackyGenerator::transform_block(parser::Block& block, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    for (std::unique_ptr<parser::BlockItem>& block_item : block.items) {
        transform_block_item(*block_item, instructions);
    }
}

std::unique_ptr<FunctionDefinition> TackyGenerator::transform_function(parser::FunctionDeclaration& function)
{
    if (function.body.has_value()) {
        std::vector<Identifier> params;
        params.reserve(function.params.size());
        for (auto& parser_param : function.params) {
            params.emplace_back(parser_param.name);
        }

        std::vector<std::unique_ptr<Instruction>> body;
        transform_block(*function.body.value().get(), body);
        body.emplace_back(std::make_unique<ReturnInstruction>(std::make_unique<Constant>(0)));
        bool global = std::get<FunctionAttribute>(m_symbol_table->symbol_at(function.name.name).attribute).global;
        return std::make_unique<FunctionDefinition>(function.name.name, global, params, std::move(body));
    }

    return nullptr;
}

std::unique_ptr<TopLevel> TackyGenerator::transform_top_level_declaration(parser::Declaration& declaration)
{
    if (parser::FunctionDeclaration* fun_decl = dynamic_cast<parser::FunctionDeclaration*>(&declaration)) {
        return transform_function(*fun_decl);
    } else {
        // Top Level VariableDeclaration are handled in a later step and converted to StaticVariable
        return nullptr;
    }
}

std::unique_ptr<Program> TackyGenerator::transform_program(parser::Program& program)
{
    std::vector<std::unique_ptr<TopLevel>> definitions;
    for (auto& parser_decl : program.declarations) {
        std::unique_ptr<TopLevel> def = transform_top_level_declaration(*parser_decl);
        if (def) {
            definitions.emplace_back(std::move(def));
        }
    }
    return std::make_unique<Program>(std::move(definitions));
}

std::string TackyGenerator::make_and_add_temporary(const Type& type, const IdentifierAttribute& attr)
{
    std::string temporary_name = m_name_generator->make_temporary();
    m_symbol_table->insert_symbol(temporary_name, type.clone(), attr);
    return temporary_name;
}
