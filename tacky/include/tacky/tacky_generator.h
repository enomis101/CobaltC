#pragma once
#include "common/data/name_generator.h"
#include "common/data/symbol_table.h"
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
    TackyGenerator(std::shared_ptr<parser::ParserAST> ast, std::shared_ptr<NameGenerator> name_generator, std::shared_ptr<SymbolTable> symbol_table);

    Program generate();

private:
    UnaryOperator transform_unary_operator(parser::UnaryOperator& op);
    BinaryOperator transform_binary_operator(parser::BinaryOperator& op);
    Value transform_expression(parser::Expression& expression, std::vector<Instruction>& instructions);
    void transform_statement(parser::Statement& statement, std::vector<Instruction>& instructions);
    void transform_declaration(parser::Declaration& declaration, std::vector<Instruction>& instructions);
    void transform_for_init(parser::ForInit& for_init, std::vector<Instruction>& instructions);
    void transform_block_item(parser::BlockItem& block_item, std::vector<Instruction>& instructions);
    void transform_block(parser::Block& block, std::vector<Instruction>& instructions);
    std::optional<FunctionDefinition> transform_function(parser::FunctionDeclaration& function);
    std::optional<TopLevel> transform_top_level_declaraiton(parser::Declaration& declaration);
    Program transform_program(parser::Program& program);

    void transform_symbols_to_tacky(Program& tacky_program);

    std::shared_ptr<parser::ParserAST> m_ast;
    std::shared_ptr<NameGenerator> m_name_generator;
    std::shared_ptr<SymbolTable> m_symbol_table;
};

} // namespace tacky
