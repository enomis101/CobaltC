#include "tacky/tacky_generator.h"
#include "parser/parser_ast.h"
#include "parser/symbol_table.h"
#include "tacky/tacky_ast.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>

using namespace tacky;

TackyGenerator::TackyGenerator(std::shared_ptr<parser::ParserAST> ast)
    : m_ast { ast }
    , m_name_generator { NameGenerator::instance() }
    , m_symbol_table { parser::SymbolTable::instance() }
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

    for (const auto& p : m_symbol_table.symbols()) {
        const auto& entry = p.second;
        if (std::holds_alternative<parser::StaticAttribute>(entry.attribute)) {
            const auto& static_attr = std::get<parser::StaticAttribute>(entry.attribute);
            bool global = static_attr.global;
            const std::string& variable_name = p.first;
            if (std::holds_alternative<parser::InitialValue>(static_attr.init)) {
                int initial_value = std::get<parser::InitialValue>(static_attr.init).value;
                top_levels.emplace_back(std::make_unique<StaticVariable>(variable_name, global, initial_value));
            } else if (std::holds_alternative<parser::TentativeInit>(static_attr.init)) {
                top_levels.emplace_back(std::make_unique<StaticVariable>(variable_name, global, 0));
            } else if (std::holds_alternative<parser::NoInit>(static_attr.init)) {
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

    throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored UnaryOperator");
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

    throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored BinaryOperator");
}

std::unique_ptr<Value> TackyGenerator::transform_expression(parser::Expression& expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ConstantExpression* constant_expression = dynamic_cast<parser::ConstantExpression*>(&expression)) {
        return std::make_unique<Constant>(constant_expression->value);
    } else if (parser::UnaryExpression* unary_expression = dynamic_cast<parser::UnaryExpression*>(&expression)) {
        std::unique_ptr<Value> src = transform_expression(*unary_expression->expression, instructions);
        std::string dst_name = m_name_generator.make_temporary();
        std::unique_ptr<TemporaryVariable> dst = std::make_unique<TemporaryVariable>(dst_name);
        std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(*dst);
        UnaryOperator op = transform_unary_operator(unary_expression->unary_operator);
        instructions.emplace_back(std::make_unique<UnaryInstruction>(op, std::move(src), std::move(dst)));
        return dst_copy;
    } else if (parser::BinaryExpression* binary_expression = dynamic_cast<parser::BinaryExpression*>(&expression)) {
        // WE need to handle logical AND OR differently to supprt short circuit
        if (binary_expression->binary_operator == parser::BinaryOperator::AND) {
            std::unique_ptr<Value> src1 = transform_expression(*binary_expression->left_expression, instructions);
            std::string false_label = m_name_generator.make_label("and_false");
            instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(src1), false_label));
            std::unique_ptr<Value> src2 = transform_expression(*binary_expression->right_expression, instructions);
            instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(src2), false_label));
            std::string result = m_name_generator.make_temporary();
            {
                std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
                instructions.emplace_back(std::make_unique<CopyInstruction>(1, std::move(result_var)));
            }

            std::string end_label = m_name_generator.make_label("and_end");
            instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
            instructions.emplace_back(std::make_unique<LabelInstruction>(false_label));
            {
                std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
                instructions.emplace_back(std::make_unique<CopyInstruction>(0, std::move(result_var)));
            }
            instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
            std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
            return result_var;
        } else if (binary_expression->binary_operator == parser::BinaryOperator::OR) {
            std::unique_ptr<Value> src1 = transform_expression(*binary_expression->left_expression, instructions);
            std::string true_label = m_name_generator.make_label("or_true");
            instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(src1), true_label));
            std::unique_ptr<Value> src2 = transform_expression(*binary_expression->right_expression, instructions);
            instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(src2), true_label));
            std::string result = m_name_generator.make_temporary();
            {
                std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
                instructions.emplace_back(std::make_unique<CopyInstruction>(0, std::move(result_var)));
            }

            std::string end_label = m_name_generator.make_label("or_end");
            instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
            instructions.emplace_back(std::make_unique<LabelInstruction>(true_label));
            {
                std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
                instructions.emplace_back(std::make_unique<CopyInstruction>(1, std::move(result_var)));
            }
            instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
            std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
            return result_var;
        } else {
            std::unique_ptr<Value> src1 = transform_expression(*binary_expression->left_expression, instructions);
            std::unique_ptr<Value> src2 = transform_expression(*binary_expression->right_expression, instructions);
            std::string dst_name = m_name_generator.make_temporary();
            std::unique_ptr<TemporaryVariable> dst = std::make_unique<TemporaryVariable>(dst_name);
            std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(*dst);
            BinaryOperator op = transform_binary_operator(binary_expression->binary_operator);
            instructions.emplace_back(std::make_unique<BinaryInstruction>(op, std::move(src1), std::move(src2), std::move(dst)));
            return dst_copy;
        }
    } else if (parser::VariableExpression* variable_expression = dynamic_cast<parser::VariableExpression*>(&expression)) {
        return std::make_unique<TemporaryVariable>(variable_expression->identifier.name);
    } else if (parser::AssignmentExpression* assignment_expression = dynamic_cast<parser::AssignmentExpression*>(&expression)) {
        parser::VariableExpression* variable = dynamic_cast<parser::VariableExpression*>(assignment_expression->left_expression.get());
        if (!variable) {
            throw TackyGeneratorError("TackyGenerator: Left side of AssignmentExpression is not a variable something went wrong with Semantic Analysis stage");
        }
        std::unique_ptr<Value> right = transform_expression(*assignment_expression->right_expression, instructions);
        instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(right), std::make_unique<TemporaryVariable>(variable->identifier.name)));
        return std::make_unique<TemporaryVariable>(variable->identifier.name);
    } else if (parser::ConditionalExpression* conditional_expression = dynamic_cast<parser::ConditionalExpression*>(&expression)) {
        // Create labels
        std::string false_label = m_name_generator.make_label("conditional_false");
        std::string end_label = m_name_generator.make_label("conditional_end");
        std::string result = m_name_generator.make_temporary();

        // Condition
        std::unique_ptr<Value> cond = transform_expression(*conditional_expression->condition, instructions);
        instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), false_label));

        // True Instructions
        std::unique_ptr<Value> true_value = transform_expression(*conditional_expression->true_expression, instructions);
        {
            // result = true_expression
            std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
            instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(true_value), std::move(result_var)));
        }
        instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));

        // False Instructions
        instructions.emplace_back(std::make_unique<LabelInstruction>(false_label));
        std::unique_ptr<Value> false_value = transform_expression(*conditional_expression->false_expression, instructions);
        {
            // result = false_expression
            std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
            instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(false_value), std::move(result_var)));
        }

        instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
        return std::make_unique<TemporaryVariable>(result);
    } else if (parser::FunctionCallExpression* function_call_expression = dynamic_cast<parser::FunctionCallExpression*>(&expression)) {

        std::vector<std::unique_ptr<Value>> args;
        for (auto& arg : function_call_expression->arguments) {
            args.emplace_back(transform_expression((*arg.get()), instructions));
        }
        std::string result = m_name_generator.make_temporary();
        std::unique_ptr<TemporaryVariable> result_var = std::make_unique<TemporaryVariable>(result);
        instructions.emplace_back(std::make_unique<FunctionCallInstruction>(function_call_expression->name.name, std::move(args), std::move(result_var)));
        return std::make_unique<TemporaryVariable>(result);
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Expression");
    }
}

void TackyGenerator::transform_statement(parser::Statement& statement, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ReturnStatement* return_statement = dynamic_cast<parser::ReturnStatement*>(&statement)) {
        std::unique_ptr<Value> value = transform_expression(*(return_statement->expression.get()), instructions);
        instructions.emplace_back(std::make_unique<ReturnInstruction>(std::move(value)));
    } else if (parser::ExpressionStatement* expression_statement = dynamic_cast<parser::ExpressionStatement*>(&statement)) {
        std::unique_ptr<Value> value = transform_expression(*(expression_statement->expression.get()), instructions);
        // value is discarded
    } else if (parser::IfStatement* if_statement = dynamic_cast<parser::IfStatement*>(&statement)) {
        std::unique_ptr<Value> cond = transform_expression(*(if_statement->condition.get()), instructions);
        if (!if_statement->else_statement.has_value()) {
            std::string end_label = m_name_generator.make_label("if_end");
            instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), end_label));
            transform_statement(*if_statement->then_statement, instructions);
            instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
        } else {
            std::string else_label = m_name_generator.make_label("else");
            std::string end_label = m_name_generator.make_label("if_end");
            instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), else_label));
            transform_statement(*if_statement->then_statement, instructions);
            instructions.emplace_back(std::make_unique<JumpInstruction>(end_label));
            instructions.emplace_back(std::make_unique<LabelInstruction>(else_label));
            transform_statement(*if_statement->else_statement.value(), instructions);
            instructions.emplace_back(std::make_unique<LabelInstruction>(end_label));
        }
        // value is discarded
    } else if (parser::CompoundStatement* compound_statement = dynamic_cast<parser::CompoundStatement*>(&statement)) {
        transform_block(*compound_statement->block.get(), instructions);
    } else if (parser::BreakStatement* break_statement = dynamic_cast<parser::BreakStatement*>(&statement)) {
        std::string break_label = "break_" + break_statement->label.name;
        instructions.emplace_back(std::make_unique<JumpInstruction>(break_label));
    } else if (parser::ContinueStatement* continue_statement = dynamic_cast<parser::ContinueStatement*>(&statement)) {
        std::string continue_label = "continue_" + continue_statement->label.name;
        instructions.emplace_back(std::make_unique<JumpInstruction>(continue_label));
    } else if (parser::DoWhileStatement* do_while_statement = dynamic_cast<parser::DoWhileStatement*>(&statement)) {
        std::string start_label = m_name_generator.make_label("do_while_start");
        std::string continue_label = "continue_" + do_while_statement->label.name;
        std::string break_label = "break_" + do_while_statement->label.name;

        instructions.emplace_back(std::make_unique<LabelInstruction>(start_label));
        transform_statement(*do_while_statement->body, instructions);
        instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));
        std::unique_ptr<Value> cond = transform_expression(*(do_while_statement->condition.get()), instructions);
        instructions.emplace_back(std::make_unique<JumpIfNotZeroInstruction>(std::move(cond), start_label));
        instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
    } else if (parser::WhileStatement* while_statement = dynamic_cast<parser::WhileStatement*>(&statement)) {
        std::string continue_label = "continue_" + while_statement->label.name;
        std::string break_label = "break_" + while_statement->label.name;
        instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));
        std::unique_ptr<Value> cond = transform_expression(*(while_statement->condition.get()), instructions);
        instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), break_label));

        transform_statement(*while_statement->body, instructions);

        instructions.emplace_back(std::make_unique<JumpInstruction>(continue_label));
        instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
    } else if (parser::ForStatement* for_statement = dynamic_cast<parser::ForStatement*>(&statement)) {
        std::string start_label = m_name_generator.make_label("for_start");
        std::string continue_label = "continue_" + for_statement->label.name;
        std::string break_label = "break_" + for_statement->label.name;
        transform_for_init(*for_statement->init, instructions);
        instructions.emplace_back(std::make_unique<LabelInstruction>(start_label));
        if (for_statement->condition.has_value()) {
            std::unique_ptr<Value> cond = transform_expression(*(for_statement->condition.value().get()), instructions);
            instructions.emplace_back(std::make_unique<JumpIfZeroInstruction>(std::move(cond), break_label));
        }

        transform_statement(*for_statement->body, instructions);
        instructions.emplace_back(std::make_unique<LabelInstruction>(continue_label));

        if (for_statement->post.has_value()) {
            transform_expression(*(for_statement->post.value().get()), instructions);
        }

        instructions.emplace_back(std::make_unique<JumpInstruction>(start_label));
        instructions.emplace_back(std::make_unique<LabelInstruction>(break_label));
    } else if (dynamic_cast<parser::NullStatement*>(&statement)) {
        // do nothing
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Statement");
    }
}

void TackyGenerator::transform_declaration(parser::Declaration& declaration, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::VariableDeclaration* variable_declaration = dynamic_cast<parser::VariableDeclaration*>(&declaration)) {
        if (variable_declaration->expression.has_value()) {
            std::unique_ptr<Value> value = transform_expression(*variable_declaration->expression.value(), instructions);
            instructions.emplace_back(std::make_unique<CopyInstruction>(std::move(value), std::make_unique<TemporaryVariable>(variable_declaration->identifier.name)));
        }
    } else if (dynamic_cast<parser::FunctionDeclaration*>(&declaration)) {
        // DO NOTHING
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Declaration");
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
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored ForInit");
    }
}

void TackyGenerator::transform_block_item(parser::BlockItem& block_item, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::Declaration* declaration = dynamic_cast<parser::Declaration*>(&block_item)) {
        transform_declaration(*declaration, instructions);
    } else if (parser::Statement* statement = dynamic_cast<parser::Statement*>(&block_item)) {
        transform_statement(*statement, instructions);
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored BlockItem");
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
        bool global = std::get<parser::FunctionAttribute>(m_symbol_table.symbols().at(function.name.name).attribute).global;
        return std::make_unique<FunctionDefinition>(function.name.name, global, params, std::move(body));
    }

    return nullptr;
}
std::unique_ptr<TopLevel> TackyGenerator::transform_top_level_declaraiton(parser::Declaration& declaration)
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
        std::unique_ptr<TopLevel> def = transform_top_level_declaraiton(*parser_decl);
        if (def) {
            definitions.emplace_back(std::move(def));
        }
    }
    return std::make_unique<Program>(std::move(definitions));
}
