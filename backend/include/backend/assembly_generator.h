#pragma once
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include "common/data/compile_options.h"
#include "common/data/name_generator.h"
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
    AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast, std::shared_ptr<SymbolTable> symbol_table, std::shared_ptr<BackendSymbolTable> backend_symbol_table, std::shared_ptr<CompileOptions> compile_options, std::shared_ptr<NameGenerator> name_generator);
    std::shared_ptr<AssemblyAST> generate();

private:
    std::unique_ptr<Operand> transform_operand(tacky::Value& op);
    UnaryOperator transform_operator(tacky::UnaryOperator& unary_operator);
    BinaryOperator transform_operator(tacky::BinaryOperator& binary_operator);
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::vector<std::unique_ptr<Instruction>> transform_return_instruction(tacky::ReturnInstruction& return_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_copy_instruction(tacky::CopyInstruction& copy_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_load_instruction(tacky::LoadInstruction& load_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_store_instruction(tacky::StoreInstruction& store_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_get_address_instruction(tacky::GetAddressInstruction& get_address_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_copy_to_offset_instruction(tacky::CopyToOffsetInstruction& copy_to_offset_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_add_pointer_instruction(tacky::AddPointerInstruction& add_pointer_instruction);

    std::vector<std::unique_ptr<Instruction>> transform_label_instruction(tacky::LabelInstruction& label_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_sign_extend_instruction(tacky::SignExtendInstruction& sign_extend_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_truncate_instruction(tacky::TruncateInstruction& truncate_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_zero_extend_instruction(tacky::ZeroExtendInstruction& zero_extend_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_int_to_double_instruction(tacky::IntToDoubleIntruction& int_to_double_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_double_to_int_instruction(tacky::DoubleToIntIntruction& double_to_int_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_uint_to_double_instruction(tacky::UIntToDoubleIntruction& uint_to_double_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_double_to_uint_instruction(tacky::DoubleToUIntIntruction& double_to_uint_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_unary_instruction(tacky::UnaryInstruction& unary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_binary_instruction(tacky::BinaryInstruction& binary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_jump_instruction(tacky::Instruction& jump_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_function_call_instruction(tacky::FunctionCallInstruction& function_call_instruction);
    std::unique_ptr<FunctionDefinition> transform_function(tacky::FunctionDefinition& function);
    std::unique_ptr<TopLevel> transform_top_level(tacky::TopLevel& top_level);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    bool is_relational_operator(tacky::BinaryOperator op);
    ConditionCode to_condition_code(tacky::BinaryOperator op, bool is_signed);
    std::pair<AssemblyType, bool> get_converted_operand_type(tacky::Value& operand);
    std::unique_ptr<Type> get_operand_type(tacky::Value& operand);
    std::pair<AssemblyType, bool> convert_type(const Type& type);
    std::shared_ptr<tacky::TackyAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
    std::shared_ptr<BackendSymbolTable> m_backend_symbol_table;
    std::shared_ptr<CompileOptions> m_compile_options;
    std::shared_ptr<NameGenerator> m_name_generator;

    void add_comment_instruction(const std::string& message, std::vector<std::unique_ptr<Instruction>>& instructions);

    const std::vector<RegisterName> INT_FUNCTION_REGISTERS;
    const std::vector<RegisterName> DOUBLE_FUNCTION_REGISTERS;

    void generate_backend_symbol_table();
    std::string add_static_double_constant(double val, size_t alignment);
    std::string get_constant_label(double val, size_t alignment);

    std::unordered_map<std::string, std::pair<std::string, std::unique_ptr<TopLevel>>> m_static_constants_map;
};

} // namespace backend
