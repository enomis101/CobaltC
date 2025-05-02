#include "tacky/tacky_generator.h"
#include <fstream>
#include <string>
#include <unordered_map>

using namespace tacky;

TackyGenerator::TackyGenerator(std::shared_ptr<parser::ParserAST> ast)
    : m_ast { ast }
    , m_name_generator { NameGenerator::instance() }
{
    if (!m_ast || !dynamic_cast<parser::Program*>(m_ast.get())) {
        throw TackyGeneratorError("TackyGenerator: Invalid AST");
    }
}

std::shared_ptr<TackyAST> TackyGenerator::generate()
{
    return transform_program(*dynamic_cast<parser::Program*>(m_ast.get()));
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
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Declaration");
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

std::unique_ptr<Function> TackyGenerator::transform_function(parser::Function& function)
{
    std::vector<std::unique_ptr<Instruction>> body;
    for (std::unique_ptr<parser::BlockItem>& block_item : function.body) {
        transform_block_item(*block_item, body);
    }
    body.emplace_back(std::make_unique<ReturnInstruction>(std::make_unique<Constant>(0)));
    return std::make_unique<Function>(function.name->name, std::move(body));
}

std::unique_ptr<Program> TackyGenerator::transform_program(parser::Program& program)
{
    return std::make_unique<Program>(transform_function(*(program.function.get())));
}
