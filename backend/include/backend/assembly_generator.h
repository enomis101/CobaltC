#pragma once
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include "common/data/symbol_table.h"
#include "tacky/tacky_ast.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace backend {

class AssemblyGeneratorError : public std::runtime_error {
public:
    explicit AssemblyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

// Generate an AssemblyAST from a TackyAST
class AssemblyGenerator {
public:
    AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast, std::shared_ptr<SymbolTable> symbol_table);

    std::shared_ptr<AssemblyAST> generate();

private:
    std::unique_ptr<Operand> transform_operand(tacky::Value& op);
    UnaryOperator transform_operator(tacky::UnaryOperator& unary_operator);
    BinaryOperator transform_operator(tacky::BinaryOperator& binary_operator);
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::vector<std::unique_ptr<Instruction>> transform_unary_instruction(tacky::UnaryInstruction& unary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_binary_instruction(tacky::BinaryInstruction& binary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_jump_instruction(tacky::Instruction& jump_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_function_call_instruction(tacky::FunctionCallInstruction& function_call_instruction);
    std::unique_ptr<FunctionDefinition> transform_function(tacky::FunctionDefinition& function);
    std::unique_ptr<TopLevel> transform_top_level(tacky::TopLevel& top_level);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    bool is_relational_operator(tacky::BinaryOperator op);
    ConditionCode to_condition_code(tacky::BinaryOperator op);
    AssemblyType get_operand_type(tacky::Value& operand);
    AssemblyType convert_type(const Type& type);
    std::shared_ptr<tacky::TackyAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
    const std::vector<RegisterName> FUN_REGISTERS;

    void generate_backend_symbol_table();

    std::shared_ptr<BackendSymbolTable> m_backend_symbol_table;
};

} // namespace backend
