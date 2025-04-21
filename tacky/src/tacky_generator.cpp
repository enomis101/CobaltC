#include "tacky/tacky_generator.h"
#include <fstream>
#include <string>

using namespace tacky;

TackyGenerator::TackyGenerator(std::shared_ptr<parser::ParserAST> ast)
    : m_ast { ast }
{
    if (!m_ast || !dynamic_cast<parser::Program*>(m_ast.get())) {
        throw TackyGeneratorError("TackyGenerator: Invalid AST");
    }
}

std::shared_ptr<TackyAST> TackyGenerator::generate()
{
    return transform_program(*dynamic_cast<parser::Program*>(m_ast.get()));
}

std::unique_ptr<UnaryOperator> TackyGenerator::transform_unary_operator(parser::UnaryOperator& unary_operator)
{
    if (dynamic_cast<parser::NegateOperator*>(&unary_operator)) {
        return std::make_unique<NegateOperator>();
    } else if (dynamic_cast<parser::ComplementOperator*>(&unary_operator)) {
        return std::make_unique<ComplementOperator>();
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored UnaryOperator");
    }
}

std::unique_ptr<Value> TackyGenerator::transform_expression(parser::Expression& expression, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (parser::ConstantExpression* constant_expression = dynamic_cast<parser::ConstantExpression*>(&expression)) {
        return std::make_unique<Constant>(constant_expression->value);
    } else if (parser::UnaryExpression* unary_expression = dynamic_cast<parser::UnaryExpression*>(&expression)) {
        std::unique_ptr<Value> src = transform_expression(*unary_expression->expression, instructions);
        std::string dst_name = make_temporary();
        std::unique_ptr<TemporaryVariable> dst = std::make_unique<TemporaryVariable>(dst_name);
        std::unique_ptr<Value> dst_copy = std::make_unique<TemporaryVariable>(*dst);
        std::unique_ptr<UnaryOperator> op = transform_unary_operator(*(unary_expression->unary_operator.get()));
        instructions.emplace_back(std::make_unique<UnaryInstruction>(std::move(op), std::move(src), std::move(dst)));
        return dst_copy;
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Expression");
    }
}

std::vector<std::unique_ptr<Instruction>> TackyGenerator::transform_statement(parser::Statement& statement)
{
    std::vector<std::unique_ptr<Instruction>> res;
    if (parser::ReturnStatement* return_statement = dynamic_cast<parser::ReturnStatement*>(&statement)) {
        std::unique_ptr<Value> value = transform_expression(*(return_statement->expression.get()), res);
        res.emplace_back(std::make_unique<ReturnInstruction>(std::move(value)));
    } else {
        throw TackyGeneratorError("TackyGenerator: Invalid or Unsuppored Statement");
    }
    return res;
}

std::unique_ptr<Function> TackyGenerator::transform_function(parser::Function& function)
{
    std::vector<std::unique_ptr<Instruction>> body = transform_statement(*(function.body.get()));
    return std::make_unique<Function>(function.name->name, std::move(body));
}

std::unique_ptr<Program> TackyGenerator::transform_program(parser::Program& program)
{
    return std::make_unique<Program>(transform_function(*(program.function.get())));
}

void TackyGenerator::reset_counter()
{
    m_temporary_counter = 0;
}

std::string TackyGenerator::make_temporary()
{
    return "tmp." + std::to_string(m_temporary_counter++);
}
