#include "backend/assembly_generator.h"
#include "backend/assembly_ast.h"

#include "backend/backend_symbol_table.h"
#include "backend/fixup_instruction_step.h"
#include "backend/pseudo_register_replace_step.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "tacky/tacky_ast.h"
#include <cassert>
#include <memory>
#include <string>
#include <variant>

using namespace backend;

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast, std::shared_ptr<SymbolTable> symbol_table, std::shared_ptr<BackendSymbolTable> backend_symbol_table)
    : m_ast { ast }
    , m_symbol_table(symbol_table)
    , m_backend_symbol_table(backend_symbol_table)
    , FUN_REGISTERS { RegisterName::DI, RegisterName::SI, RegisterName::DX, RegisterName::CX, RegisterName::R8, RegisterName::R9 }
{
    if (!m_ast || !dynamic_cast<tacky::Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid AST");
    }
}

void AssemblyGenerator::generate_backend_symbol_table()
{
    for (const auto& st_entry : m_symbol_table->symbols()) {
        const auto& symbol_name = st_entry.first;
        if (dynamic_cast<FunctionType*>(st_entry.second.type.get())) {
            assert(std::holds_alternative<FunctionAttribute>(st_entry.second.attribute));
            const auto& function_attribute = std::get<FunctionAttribute>(st_entry.second.attribute);
            m_backend_symbol_table->insert_symbol(symbol_name, FunctionEntry { 0, function_attribute.defined });
        } else {
            AssemblyType type = convert_type(*st_entry.second.type);
            bool is_static = std::holds_alternative<StaticAttribute>(st_entry.second.attribute);
            m_backend_symbol_table->insert_symbol(symbol_name, ObjectEntry { type, is_static });
        }
    }
}

std::shared_ptr<AssemblyAST> AssemblyGenerator::generate()
{
    std::shared_ptr<AssemblyAST> m_assembly_ast = transform_program(*dynamic_cast<tacky::Program*>(m_ast.get()));
    generate_backend_symbol_table();

    PseudoRegisterReplaceStep step1(m_assembly_ast, m_backend_symbol_table);
    step1.replace();
    FixUpInstructionsStep step2(m_assembly_ast, m_backend_symbol_table);
    step2.fixup();
    return m_assembly_ast;
}

std::unique_ptr<Operand> AssemblyGenerator::transform_operand(tacky::Value& val)
{
    if (tacky::Constant* constant = dynamic_cast<tacky::Constant*>(&val)) {
        return std::make_unique<ImmediateValue>(constant->value);
    } else if (tacky::TemporaryVariable* var = dynamic_cast<tacky::TemporaryVariable*>(&val)) {
        return std::make_unique<PseudoRegister>(var->identifier.name);
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::Value");
    }
}

UnaryOperator AssemblyGenerator::transform_operator(tacky::UnaryOperator& unary_operator)
{
    static const std::unordered_map<tacky::UnaryOperator, UnaryOperator> unary_op_map = {
        { tacky::UnaryOperator::NEGATE, UnaryOperator::NEG },
        { tacky::UnaryOperator::COMPLEMENT, UnaryOperator::NOT }
    };

    auto it = unary_op_map.find(unary_operator);
    if (it == unary_op_map.end()) {
        assert(false && "AssemblyGenerator: Invalid or Unsupported tacky::UnaryOperator");
    }
    return it->second;
}

BinaryOperator AssemblyGenerator::transform_operator(tacky::BinaryOperator& binary_operator)
{
    static const std::unordered_map<tacky::BinaryOperator, BinaryOperator> binary_op_map = {
        { tacky::BinaryOperator::ADD, BinaryOperator::ADD },
        { tacky::BinaryOperator::SUBTRACT, BinaryOperator::SUB },
        { tacky::BinaryOperator::MULTIPLY, BinaryOperator::MULT }
    };

    auto it = binary_op_map.find(binary_operator);
    if (it == binary_op_map.end()) {
        assert(false && "AssemblyGenerator: Invalid or Unsupported tacky::BinaryOperator");
    }
    return it->second;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_instruction(tacky::Instruction& instruction)
{
    if (tacky::ReturnInstruction* return_instruction = dynamic_cast<tacky::ReturnInstruction*>(&instruction)) {
        return transform_return_instruction(*return_instruction);
    } else if (tacky::UnaryInstruction* unary_instruction = dynamic_cast<tacky::UnaryInstruction*>(&instruction)) {
        return transform_unary_instruction(*unary_instruction);
    } else if (tacky::BinaryInstruction* binary_instruction = dynamic_cast<tacky::BinaryInstruction*>(&instruction)) {
        return transform_binary_instruction(*binary_instruction);
    } else if (dynamic_cast<tacky::JumpInstruction*>(&instruction) || dynamic_cast<tacky::JumpIfZeroInstruction*>(&instruction) || dynamic_cast<tacky::JumpIfNotZeroInstruction*>(&instruction)) {
        return transform_jump_instruction(instruction);
    } else if (tacky::CopyInstruction* copy_instruction = dynamic_cast<tacky::CopyInstruction*>(&instruction)) {
        return transform_copy_instruction(*copy_instruction);
    } else if (tacky::LabelInstruction* label_instruction = dynamic_cast<tacky::LabelInstruction*>(&instruction)) {
        return transform_label_instruction(*label_instruction);
    } else if (tacky::FunctionCallInstruction* function_call_instruction = dynamic_cast<tacky::FunctionCallInstruction*>(&instruction)) {
        return transform_function_call_instruction(*function_call_instruction);
    } else if (tacky::SignExtendInstruction* sign_extend_instruction = dynamic_cast<tacky::SignExtendInstruction*>(&instruction)) {
        return transform_sign_extend_instruction(*sign_extend_instruction);
    } else if (tacky::TruncateInstruction* truncate_instruction = dynamic_cast<tacky::TruncateInstruction*>(&instruction)) {
        return transform_truncate_instruction(*truncate_instruction);
    } else {
        assert(false && "AssemblyGenerator: Invalid or Unsupported tacky::Instruction");
    }
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_return_instruction(tacky::ReturnInstruction& return_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    AssemblyType value_type = get_operand_type(*return_instruction.value);
    std::unique_ptr<Operand> src = transform_operand(*return_instruction.value);
    std::unique_ptr<Operand> dst = std::make_unique<Register>(RegisterName::AX);
    res.emplace_back(std::make_unique<MovInstruction>(value_type, std::move(src), std::move(dst)));
    res.emplace_back(std::make_unique<ReturnInstruction>());
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_copy_instruction(tacky::CopyInstruction& copy_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    AssemblyType type = get_operand_type(*copy_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*copy_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*copy_instruction.destination);
    res.emplace_back(std::make_unique<MovInstruction>(type, std::move(src), std::move(dst)));
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_label_instruction(tacky::LabelInstruction& label_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    res.emplace_back(std::make_unique<LabelInstruction>(label_instruction.identifier.name));
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_sign_extend_instruction(tacky::SignExtendInstruction& sign_extend_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    std::unique_ptr<Operand> src = transform_operand(*sign_extend_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*sign_extend_instruction.destination);
    res.emplace_back(std::make_unique<MovsxInstruction>(std::move(src), std::move(dst)));
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_truncate_instruction(tacky::TruncateInstruction& truncate_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    std::unique_ptr<Operand> src = transform_operand(*truncate_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*truncate_instruction.destination);
    res.emplace_back(std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(src), std::move(dst)));
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_unary_instruction(tacky::UnaryInstruction& unary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    AssemblyType source_type = get_operand_type(*unary_instruction.source);
    AssemblyType destination_type = get_operand_type(*unary_instruction.destination);
    if (unary_instruction.unary_operator == tacky::UnaryOperator::NOT) {

        std::unique_ptr<Operand> src = transform_operand(*unary_instruction.source);
        std::unique_ptr<Operand> dst = transform_operand(*unary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();

        res.emplace_back(std::make_unique<CmpInstruction>(source_type, std::make_unique<ImmediateValue>(0), std::move(src)));
        res.emplace_back(std::make_unique<MovInstruction>(destination_type, std::make_unique<ImmediateValue>(0), std::move(dst)));
        res.emplace_back(std::make_unique<SetCCInstruction>(ConditionCode::E, std::move(dst_copy)));
    } else {
        std::unique_ptr<Operand> src = transform_operand(*unary_instruction.source);
        std::unique_ptr<Operand> dst = transform_operand(*unary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<MovInstruction>(source_type, std::move(src), std::move(dst)));

        UnaryOperator op = transform_operator(unary_instruction.unary_operator);
        res.emplace_back(std::make_unique<UnaryInstruction>(op, source_type, std::move(dst_copy)));
    }
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_binary_instruction(tacky::BinaryInstruction& binary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    AssemblyType source1_type = get_operand_type(*binary_instruction.source1);
    AssemblyType destination_type = get_operand_type(*binary_instruction.destination);
    if (is_relational_operator(binary_instruction.binary_operator)) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<CmpInstruction>(source1_type, std::move(src2), std::move(src1)));
        res.emplace_back(std::make_unique<MovInstruction>(destination_type, std::make_unique<ImmediateValue>(0), std::move(dst)));
        res.emplace_back(std::make_unique<SetCCInstruction>(to_condition_code(binary_instruction.binary_operator), std::move(dst_copy)));
    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::DIVIDE) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        res.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::make_unique<Register>(RegisterName::AX)));
        res.emplace_back(std::make_unique<CdqInstruction>(source1_type));
        res.emplace_back(std::make_unique<IdivInstruction>(source1_type, std::move(src2)));
        res.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<Register>(RegisterName::AX), std::move(dst)));
    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::REMAINDER) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        res.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::make_unique<Register>(RegisterName::AX)));
        res.emplace_back(std::make_unique<CdqInstruction>(source1_type));
        res.emplace_back(std::make_unique<IdivInstruction>(source1_type, std::move(src2)));
        res.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<Register>(RegisterName::DX), std::move(dst)));
    } else {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::move(dst)));

        BinaryOperator op = transform_operator(binary_instruction.binary_operator);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        res.emplace_back(std::make_unique<BinaryInstruction>(op, source1_type, std::move(src2), std::move(dst_copy)));
    }
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_jump_instruction(tacky::Instruction& instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    if (tacky::JumpInstruction* jump_instruction = dynamic_cast<tacky::JumpInstruction*>(&instruction)) {
        res.emplace_back(std::make_unique<JmpInstruction>(jump_instruction->identifier.name));
    } else if (tacky::JumpIfZeroInstruction* jump_if_zero_instruction = dynamic_cast<tacky::JumpIfZeroInstruction*>(&instruction)) {
        AssemblyType condition_type = get_operand_type(*jump_if_zero_instruction->condition);
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_zero_instruction->condition);
        res.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<ImmediateValue>(0), std::move(cond)));
        res.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::E, jump_if_zero_instruction->identifier.name));
    } else if (tacky::JumpIfNotZeroInstruction* jump_if_not_zero_instruction = dynamic_cast<tacky::JumpIfNotZeroInstruction*>(&instruction)) {
        AssemblyType condition_type = get_operand_type(*jump_if_not_zero_instruction->condition);
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_not_zero_instruction->condition);
        res.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<ImmediateValue>(0), std::move(cond)));
        res.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::NE, jump_if_not_zero_instruction->identifier.name));
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator::transform_jump_instruction Invalid or Unsupported tacky::Instruction");
    }
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_function_call_instruction(tacky::FunctionCallInstruction& function_call_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;

    std::vector<std::unique_ptr<tacky::Value>>& tacky_arguments = function_call_instruction.arguments;
    const size_t max_reg_args_size = FUN_REGISTERS.size();
    const size_t reg_args_size = std::min(max_reg_args_size, tacky_arguments.size());
    const size_t stack_args_size = std::max(0, (int)tacky_arguments.size() - (int)max_reg_args_size);

    // adjust stack alignment
    int stack_padding = 0;
    if (stack_args_size % 2 != 0) {
        stack_padding = 8;
    }

    if (stack_padding != 0) {
        res.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::SUB, AssemblyType::QUAD_WORD,
            std::make_unique<ImmediateValue>(stack_padding), std::make_unique<Register>(RegisterName::SP)));
    }

    for (size_t i = 0; i < reg_args_size; ++i) {
        RegisterName reg_name = FUN_REGISTERS[i];
        AssemblyType arg_type = get_operand_type(*tacky_arguments[i].get());
        std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[i].get());

        res.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(assembly_arg), std::make_unique<Register>(reg_name)));
    }

    for (size_t i = 0; i < stack_args_size; ++i) {
        int j = tacky_arguments.size() - 1 - i; // reverse
        AssemblyType arg_type = get_operand_type(*tacky_arguments[j].get());
        std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[j].get());
        if (dynamic_cast<Register*>(assembly_arg.get()) || dynamic_cast<ImmediateValue*>(assembly_arg.get()) || arg_type == AssemblyType::QUAD_WORD) {
            res.emplace_back(std::make_unique<PushInstruction>(std::move(assembly_arg)));
        } else {
            // Because we can run into trouble if we push a 4-byte operand from memory into the stack we first move it into AX
            res.emplace_back(std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(assembly_arg), std::make_unique<Register>(RegisterName::AX)));
            res.emplace_back(std::make_unique<PushInstruction>(std::make_unique<Register>(RegisterName::AX)));
        }
    }

    // emit call instruciton
    res.emplace_back(std::make_unique<CallInstruction>(function_call_instruction.name.name));

    // adjust stack pointer
    int bytes_to_remove = 8 * stack_args_size + stack_padding;
    if (bytes_to_remove != 0) {
        res.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::ADD, AssemblyType::QUAD_WORD,
            std::make_unique<ImmediateValue>(bytes_to_remove), std::make_unique<Register>(RegisterName::SP)));
    }

    // retrieve return value
    std::unique_ptr<Operand> dst = transform_operand(*function_call_instruction.destination);
    AssemblyType dst_type = get_operand_type(*function_call_instruction.destination);
    res.emplace_back(std::make_unique<MovInstruction>(dst_type, std::make_unique<Register>(RegisterName::AX), std::move(dst)));
    return res;
}

std::unique_ptr<FunctionDefinition> AssemblyGenerator::transform_function(tacky::FunctionDefinition& function_definition)
{
    std::vector<std::unique_ptr<Instruction>> instructions;

    std::vector<std::string> tacky_parameters;
    for (auto& par : function_definition.parameters) {
        tacky_parameters.emplace_back(par.name);
    }

    const auto& symbol = m_symbol_table->symbol_at(function_definition.name.name);
    auto function_type = dynamic_cast<const FunctionType*>(symbol.type.get());
    assert(function_type && "Invalid Function type at AssemblyGenerator::transform_function");
    const auto& arg_types = function_type->parameters_type;

    const size_t max_reg_args_size = FUN_REGISTERS.size();
    const size_t reg_args_size = std::min(max_reg_args_size, tacky_parameters.size());

    // move register parameters from registers to pseudo registers
    for (size_t i = 0; i < reg_args_size; ++i) {
        AssemblyType arg_type = convert_type(*arg_types.at(i));

        std::unique_ptr<Register> reg = std::make_unique<Register>(FUN_REGISTERS[i]);
        std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
        instructions.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(reg), std::move(pseudo_reg)));
    }

    int stack_offset = 16;
    for (size_t i = reg_args_size; i < tacky_parameters.size(); ++i, stack_offset += 8) {
        AssemblyType arg_type = convert_type(*arg_types.at(i));
        std::unique_ptr<StackAddress> stack_addr = std::make_unique<StackAddress>(stack_offset);
        std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
        instructions.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(stack_addr), std::move(pseudo_reg)));
    }

    for (auto& i : function_definition.body) {
        std::vector<std::unique_ptr<Instruction>> tmp_instrucitons = transform_instruction(*i);
        for (auto& tmp_i : tmp_instrucitons) {
            instructions.push_back(std::move(tmp_i));
        }
    }
    return std::make_unique<FunctionDefinition>(function_definition.name.name, function_definition.global, std::move(instructions));
}

std::unique_ptr<TopLevel> AssemblyGenerator::transform_top_level(tacky::TopLevel& top_level)
{
    if (tacky::FunctionDefinition* fun = dynamic_cast<tacky::FunctionDefinition*>(&top_level)) {
        return transform_function(*fun);
    } else if (tacky::StaticVariable* static_var = dynamic_cast<tacky::StaticVariable*>(&top_level)) {
        return std::make_unique<StaticVariable>(static_var->name.name, static_var->global, static_var->type->alignment(), static_var->init);
    } else {
        throw AssemblyGeneratorError("In transform_top_level: invalid top level class");
    }
}

std::unique_ptr<Program> AssemblyGenerator::transform_program(tacky::Program& program)
{
    std::vector<std::unique_ptr<TopLevel>> definitions;

    for (auto& def : program.definitions) {
        definitions.emplace_back(transform_top_level(*def));
    }

    return std::make_unique<Program>(std::move(definitions));
}

bool AssemblyGenerator::is_relational_operator(tacky::BinaryOperator op)
{
    switch (op) {
    case tacky::BinaryOperator::EQUAL:
    case tacky::BinaryOperator::NOT_EQUAL:
    case tacky::BinaryOperator::LESS_THAN:
    case tacky::BinaryOperator::LESS_OR_EQUAL:
    case tacky::BinaryOperator::GREATER_THAN:
    case tacky::BinaryOperator::GREATER_OR_EQUAL:
        return true;
    default:
        return false;
    }
}

ConditionCode AssemblyGenerator::to_condition_code(tacky::BinaryOperator op)
{
    switch (op) {
    case tacky::BinaryOperator::EQUAL:
        return ConditionCode::E;
    case tacky::BinaryOperator::NOT_EQUAL:
        return ConditionCode::NE;
    case tacky::BinaryOperator::LESS_THAN:
        return ConditionCode::L;
    case tacky::BinaryOperator::LESS_OR_EQUAL:
        return ConditionCode::LE;
    case tacky::BinaryOperator::GREATER_THAN:
        return ConditionCode::G;
    case tacky::BinaryOperator::GREATER_OR_EQUAL:
        return ConditionCode::GE;
    default:
        throw AssemblyGeneratorError("AssemblyGenerator::to_condition_code is not a relational operator");
    }
}

AssemblyType AssemblyGenerator::get_operand_type(tacky::Value& operand)
{
    if (tacky::Constant* tacky_constant = dynamic_cast<tacky::Constant*>(&operand)) {
        if (std::holds_alternative<int>(tacky_constant->value)) {
            return AssemblyType::LONG_WORD;
        } else if (std::holds_alternative<long>(tacky_constant->value)) {
            return AssemblyType::QUAD_WORD;
        }
        assert(false && "tacky::Constant.value type not supported");
    } else if (tacky::TemporaryVariable* tacky_var = dynamic_cast<tacky::TemporaryVariable*>(&operand)) {
        const auto& symbol = m_symbol_table->symbol_at(tacky_var->identifier.name);
        return convert_type(*symbol.type);
    }
    assert(false && "Invalid tacky::Value");
}

AssemblyType AssemblyGenerator::convert_type(const Type& type)
{
    if (dynamic_cast<const IntType*>(&type)) {
        return AssemblyType::LONG_WORD;
    } else if (dynamic_cast<const LongType*>(&type)) {
        return AssemblyType::QUAD_WORD;
    }
    assert(false && "Unsupported type for Type to AssemblyType conversion");
}
