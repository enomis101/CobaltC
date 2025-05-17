#pragma once
#include "common/data/name_generator.h"
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
    void transform_statement(parser::Statement& statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_declaration(parser::Declaration& declaration, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_for_init(parser::ForInit& for_init, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_block_item(parser::BlockItem& block_item, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_block(parser::Block& block, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<FunctionDefinition> transform_function(parser::FunctionDeclaration& function);
    std::unique_ptr<Program> transform_program(parser::Program& program);

    std::shared_ptr<parser::ParserAST> m_ast;
    NameGenerator& m_name_generator;
};

} // namespace tacky
