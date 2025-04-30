#pragma once
#include "parser/parser_ast.h"
#include "tacky/tacky_ast.h"
#include <stdexcept>
#include <vector>
namespace tacky {

class TackyGeneratorError : public std::runtime_error {
public:
    explicit TackyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

// Generate an TackyAST from a ParserAST
class TackyGenerator {
public:
    TackyGenerator(std::shared_ptr<parser::ParserAST> ast);

    std::shared_ptr<TackyAST> generate();

private:
    UnaryOperator transform_unary_operator(parser::UnaryOperator& op);
    BinaryOperator transform_binary_operator(parser::BinaryOperator& op);
    std::unique_ptr<Value> transform_expression(parser::Expression& expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::vector<std::unique_ptr<Instruction>> transform_statement(parser::Statement& statement);
    std::unique_ptr<Function> transform_function(parser::Function& function);
    std::unique_ptr<Program> transform_program(parser::Program& program);
    std::string make_temporary();
    std::string make_label(const std::string& in_label);
    void reset_counter();

    std::shared_ptr<parser::ParserAST> m_ast;
    int m_temporary_counter = 0;
    int m_label_counter = 0;
};

} // namespace tacky
