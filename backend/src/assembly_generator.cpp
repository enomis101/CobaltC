#include "backend/assembly_generator.h"
#include "backend/assembly_ast.h"

#include "backend/assembly_printer.h"
#include "backend/backend_symbol_table.h"
#include "backend/fixup_instruction_step.h"
#include "backend/pseudo_register_replace_step.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "common/error/internal_compiler_error.h"
#include "tacky/tacky_ast.h"
#include "tacky/tacky_printer.h"
#include <cassert>
#include <format>
#include <memory>
#include <ranges>
#include <string>
#include <tuple>
#include <variant>

using namespace backend;

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast, std::shared_ptr<SymbolTable> symbol_table, std::shared_ptr<BackendSymbolTable> backend_symbol_table, std::shared_ptr<CompileOptions> compile_options, std::shared_ptr<NameGenerator> name_generator)
    : m_ast { ast }
    , m_symbol_table(symbol_table)
    , m_backend_symbol_table(backend_symbol_table)
    , m_compile_options(compile_options)
    , m_name_generator(name_generator)
    , INT_FUNCTION_REGISTERS { RegisterName::DI, RegisterName::SI, RegisterName::DX, RegisterName::CX, RegisterName::R8, RegisterName::R9 }
    , DOUBLE_FUNCTION_REGISTERS { RegisterName::XMM0, RegisterName::XMM1, RegisterName::XMM2, RegisterName::XMM3, RegisterName::XMM4, RegisterName::XMM5, RegisterName::XMM6, RegisterName::XMM7 }
{
    if (!m_ast || !dynamic_cast<tacky::Program*>(m_ast.get())) {
        assert(false && "AssemblyGenerator: Invalid AST");
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
            auto [type, _] = convert_type(*st_entry.second.type);
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
        if (std::holds_alternative<double>(constant->value)) {
            auto double_val = std::get<double>(constant->value);
            std::string constant_label = add_static_double_constant(double_val, 8);
            return std::make_unique<DataOperand>(constant_label);
        } else {
            return std::make_unique<ImmediateValue>(constant->value);
        }

    } else if (tacky::TemporaryVariable* var = dynamic_cast<tacky::TemporaryVariable*>(&val)) {
        const auto& symbol = m_symbol_table->symbol_at(var->identifier.name);
        if (symbol.type->is_scalar()) {
            return std::make_unique<PseudoRegister>(var->identifier.name);
        } else {
            return std::make_unique<PseudoMemory>(var->identifier.name, 0);
        }

    } else {
        assert(false && "AssemblyGenerator: Invalid or Unsupported tacky::Value");
        return nullptr;
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
    } else if (tacky::ZeroExtendInstruction* zero_extend_instruction = dynamic_cast<tacky::ZeroExtendInstruction*>(&instruction)) {
        return transform_zero_extend_instruction(*zero_extend_instruction);
    } else if (tacky::IntToDoubleIntruction* int_to_double_instruction = dynamic_cast<tacky::IntToDoubleIntruction*>(&instruction)) {
        return transform_int_to_double_instruction(*int_to_double_instruction);
    } else if (tacky::DoubleToIntIntruction* double_to_int_instruction = dynamic_cast<tacky::DoubleToIntIntruction*>(&instruction)) {
        return transform_double_to_int_instruction(*double_to_int_instruction);
    } else if (tacky::UIntToDoubleIntruction* uint_to_double_instruction = dynamic_cast<tacky::UIntToDoubleIntruction*>(&instruction)) {
        return transform_uint_to_double_instruction(*uint_to_double_instruction);
    } else if (tacky::DoubleToUIntIntruction* double_to_uint_instruction = dynamic_cast<tacky::DoubleToUIntIntruction*>(&instruction)) {
        return transform_double_to_uint_instruction(*double_to_uint_instruction);
    } else if (auto load_intruction = dynamic_cast<tacky::LoadInstruction*>(&instruction)) {
        return transform_load_instruction(*load_intruction);
    } else if (auto store_instruction = dynamic_cast<tacky::StoreInstruction*>(&instruction)) {
        return transform_store_instruction(*store_instruction);
    } else if (auto get_address_instruction = dynamic_cast<tacky::GetAddressInstruction*>(&instruction)) {
        return transform_get_address_instruction(*get_address_instruction);
    } else if (auto copy_to_offset_instruction = dynamic_cast<tacky::CopyToOffsetInstruction*>(&instruction)) {
        return transform_copy_to_offset_instruction(*copy_to_offset_instruction);
    } else if (auto add_pointer_instruction = dynamic_cast<tacky::AddPointerInstruction*>(&instruction)) {
        return transform_add_pointer_instruction(*add_pointer_instruction);
    } else {
        throw InternalCompilerError("AssemblyGenerator: Invalid or Unsupported tacky::Instruction");
    }
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_return_instruction(tacky::ReturnInstruction& return_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto [value_type, _] = get_operand_type(*return_instruction.value);
    bool is_double = (value_type == AssemblyType::DOUBLE);
    std::unique_ptr<Operand> src = transform_operand(*return_instruction.value);
    std::unique_ptr<Operand> dst = std::make_unique<Register>(is_double ? RegisterName::XMM0 : RegisterName::AX);
    add_comment_instruction("return_instruction", instructions);
    if (value_type == AssemblyType::BYTE_ARRAY) {
        tacky::PrinterVisitor printer;
        printer.generate_dot_file("debug/test_tacky.dot", return_instruction);
    }

    instructions.emplace_back(std::make_unique<MovInstruction>(value_type, std::move(src), std::move(dst)));
    instructions.emplace_back(std::make_unique<ReturnInstruction>());
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_copy_instruction(tacky::CopyInstruction& copy_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto [type, _] = get_operand_type(*copy_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*copy_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*copy_instruction.destination);
    add_comment_instruction("copy_instruction", instructions);
    instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_load_instruction(tacky::LoadInstruction& load_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto [dst_type, _] = get_operand_type(*load_instruction.destination);
    std::unique_ptr<Operand> src_ptr = transform_operand(*load_instruction.source_pointer);
    std::unique_ptr<Operand> dst = transform_operand(*load_instruction.destination);
    add_comment_instruction("load_instruction", instructions);
    // use QUAD_WORD as we are copying a pointer into a register
    auto reg = std::make_unique<Register>(RegisterName::AX);
    instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, src_ptr->clone(), reg->clone()));
    instructions.emplace_back(std::make_unique<MovInstruction>(dst_type, std::make_unique<MemoryAddress>(reg->name, 0), dst->clone()));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_store_instruction(tacky::StoreInstruction& store_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto [src_type, _] = get_operand_type(*store_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*store_instruction.source);
    std::unique_ptr<Operand> dst_ptr = transform_operand(*store_instruction.destination_pointer);
    add_comment_instruction("store_instruction", instructions);
    // use QUAD_WORD as we are copying a pointer into a register
    auto reg = std::make_unique<Register>(RegisterName::AX);
    instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, dst_ptr->clone(), reg->clone()));
    instructions.emplace_back(std::make_unique<MovInstruction>(src_type, src->clone(), std::make_unique<MemoryAddress>(reg->name, 0)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_get_address_instruction(tacky::GetAddressInstruction& get_address_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src = transform_operand(*get_address_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*get_address_instruction.destination);
    add_comment_instruction("get_address_instruction", instructions);
    instructions.emplace_back(std::make_unique<LeaInstruction>(std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_copy_to_offset_instruction(tacky::CopyToOffsetInstruction& copy_to_offset_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src = transform_operand(*copy_to_offset_instruction.source);
    auto [src_type, _] = get_operand_type(*copy_to_offset_instruction.source);
    std::unique_ptr<Operand> pseudo_mem = std::make_unique<PseudoMemory>(copy_to_offset_instruction.identifier.name, copy_to_offset_instruction.offset);
    add_comment_instruction("copy_to_offset_instruction", instructions);
    instructions.emplace_back(std::make_unique<MovInstruction>(src_type, std::move(src), std::move(pseudo_mem)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_add_pointer_instruction(tacky::AddPointerInstruction& add_pointer_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src_ptr = transform_operand(*add_pointer_instruction.source_pointer);
    std::unique_ptr<Operand> idx = transform_operand(*add_pointer_instruction.index);
    std::unique_ptr<Operand> dst = transform_operand(*add_pointer_instruction.destination);
    add_comment_instruction("add_pointer_instruction", instructions);

    if (auto imm_val = dynamic_cast<ImmediateValue*>(idx.get())) {
        long res = std::visit([&](const auto& val) -> long {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, int>) {
                return static_cast<long>(val) * add_pointer_instruction.scale;
            } else if constexpr (std::is_same_v<T, long>) {
                return val * add_pointer_instruction.scale;
            } else if constexpr (std::is_same_v<T, unsigned int>) {
                return static_cast<long>(val) * add_pointer_instruction.scale;
            } else if constexpr (std::is_same_v<T, unsigned long>) {
                return static_cast<long>(val) * add_pointer_instruction.scale;
            } else {
                // std::monostate or double - invalid for index
                throw InternalCompilerError("Invalid index type: only integer types allowed");
            }
        },
            imm_val->value);
        std::unique_ptr<Operand> mem = std::make_unique<MemoryAddress>(RegisterName::AX, res);
        std::unique_ptr<Operand> reg = std::make_unique<Register>(RegisterName::AX);
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(src_ptr), std::move(reg)));
        instructions.emplace_back(std::make_unique<LeaInstruction>(std::move(mem), std::move(dst)));

    } else {
        if (add_pointer_instruction.scale == 1 || add_pointer_instruction.scale == 2 || add_pointer_instruction.scale == 4 || add_pointer_instruction.scale == 8) {
            std::unique_ptr<Register> reg1 = std::make_unique<Register>(RegisterName::AX);
            std::unique_ptr<Register> reg2 = std::make_unique<Register>(RegisterName::DX);

            instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(src_ptr), reg1->clone()));
            instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(idx), reg2->clone()));
            std::unique_ptr<Operand> idx_addr = std::make_unique<IndexedAddress>(std::move(reg1), std::move(reg2), add_pointer_instruction.scale);
            instructions.emplace_back(std::make_unique<LeaInstruction>(std::move(idx_addr), std::move(dst)));
        } else {
            std::unique_ptr<Register> reg1 = std::make_unique<Register>(RegisterName::AX);
            std::unique_ptr<Register> reg2 = std::make_unique<Register>(RegisterName::DX);

            instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(src_ptr), reg1->clone()));
            instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(idx), reg2->clone()));
            auto imm = std::make_unique<ImmediateValue>(add_pointer_instruction.scale);
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::MULT, AssemblyType::QUAD_WORD, std::move(imm), reg2->clone()));

            std::unique_ptr<Operand> idx_addr = std::make_unique<IndexedAddress>(std::move(reg1), std::move(reg2), 1);
            instructions.emplace_back(std::make_unique<LeaInstruction>(std::move(idx_addr), std::move(dst)));
        }
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_label_instruction(tacky::LabelInstruction& label_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    // no comment is needed
    instructions.emplace_back(std::make_unique<LabelInstruction>(label_instruction.identifier.name));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_sign_extend_instruction(tacky::SignExtendInstruction& sign_extend_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src = transform_operand(*sign_extend_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*sign_extend_instruction.destination);
    add_comment_instruction("sign_extend_instruction", instructions);
    instructions.emplace_back(std::make_unique<MovsxInstruction>(std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_truncate_instruction(tacky::TruncateInstruction& truncate_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src = transform_operand(*truncate_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*truncate_instruction.destination);
    add_comment_instruction("truncate_instruction", instructions);
    instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_zero_extend_instruction(tacky::ZeroExtendInstruction& zero_extend_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::unique_ptr<Operand> src = transform_operand(*zero_extend_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*zero_extend_instruction.destination);
    add_comment_instruction("zero_extend_instruction", instructions);
    instructions.emplace_back(std::make_unique<MovZeroExtendInstruction>(std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_int_to_double_instruction(tacky::IntToDoubleIntruction& int_to_double_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    add_comment_instruction("int_to_double_instruction", instructions);
    auto [src_type, _] = get_operand_type(*int_to_double_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*int_to_double_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*int_to_double_instruction.destination);

    instructions.emplace_back(std::make_unique<Cvtsi2sdInstruction>(src_type, std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_double_to_int_instruction(tacky::DoubleToIntIntruction& double_to_int_instruction)
{

    std::vector<std::unique_ptr<Instruction>> instructions;
    add_comment_instruction("double_to_int_instruction", instructions);
    auto [dst_type, _] = get_operand_type(*double_to_int_instruction.destination);
    std::unique_ptr<Operand> src = transform_operand(*double_to_int_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*double_to_int_instruction.destination);

    instructions.emplace_back(std::make_unique<Cvttsd2siInstruction>(dst_type, std::move(src), std::move(dst)));
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_uint_to_double_instruction(tacky::UIntToDoubleIntruction& uint_to_double_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    add_comment_instruction("uint_to_double_instruction", instructions);
    auto [src_type, _] = get_operand_type(*uint_to_double_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*uint_to_double_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*uint_to_double_instruction.destination);
    RegisterName reg1_name = RegisterName::AX;
    auto reg1 = std::make_unique<Register>(reg1_name);
    RegisterName reg2_name = RegisterName::DX;
    auto reg2 = std::make_unique<Register>(reg2_name);
    if (src_type == AssemblyType::LONG_WORD) {
        instructions.emplace_back(std::make_unique<MovZeroExtendInstruction>(src->clone(), reg1->clone()));
        instructions.emplace_back(std::make_unique<Cvtsi2sdInstruction>(AssemblyType::QUAD_WORD, reg1->clone(), dst->clone()));
    } else {
        std::string label1 = m_name_generator->make_label("uint_to_double");
        std::string label2 = m_name_generator->make_label("uint_to_double");
        instructions.emplace_back(std::make_unique<CmpInstruction>(AssemblyType::QUAD_WORD, std::make_unique<ImmediateValue>(0), src->clone()));
        instructions.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::L, label1));
        instructions.emplace_back(std::make_unique<Cvtsi2sdInstruction>(AssemblyType::QUAD_WORD, src->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<JmpInstruction>(label2));
        instructions.emplace_back(std::make_unique<LabelInstruction>(label1));
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, src->clone(), reg1->clone()));
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, reg1->clone(), reg2->clone()));
        instructions.emplace_back(std::make_unique<UnaryInstruction>(UnaryOperator::SHR, AssemblyType::QUAD_WORD, reg2->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::AND, AssemblyType::QUAD_WORD, std::make_unique<ImmediateValue>(1), reg1->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::OR, AssemblyType::QUAD_WORD, reg1->clone(), reg2->clone()));
        instructions.emplace_back(std::make_unique<Cvtsi2sdInstruction>(AssemblyType::QUAD_WORD, reg2->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::ADD, AssemblyType::DOUBLE, dst->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<LabelInstruction>(label2));
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_double_to_uint_instruction(tacky::DoubleToUIntIntruction& double_to_uint_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    add_comment_instruction("double_to_uint_instruction", instructions);
    auto [src_type, _] = get_operand_type(*double_to_uint_instruction.source);
    std::unique_ptr<Operand> src = transform_operand(*double_to_uint_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*double_to_uint_instruction.destination);
    RegisterName regr_name = RegisterName::AX;
    auto regr = std::make_unique<Register>(regr_name);
    RegisterName regx_name = RegisterName::XMM0;
    auto regx = std::make_unique<Register>(regx_name);
    if (src_type == AssemblyType::LONG_WORD) {
        instructions.emplace_back(std::make_unique<Cvttsd2siInstruction>(AssemblyType::QUAD_WORD, src->clone(), regr->clone()));
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, regr->clone(), dst->clone()));
    } else {
        std::string label1 = m_name_generator->make_label("uint_to_double");
        std::string label2 = m_name_generator->make_label("uint_to_double");
        std::string upper_bound_label = add_static_double_constant(9223372036854775808.0, 8);

        instructions.emplace_back(std::make_unique<CmpInstruction>(AssemblyType::QUAD_WORD, std::make_unique<DataOperand>(upper_bound_label), src->clone()));
        instructions.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::AE, label1));
        instructions.emplace_back(std::make_unique<Cvttsd2siInstruction>(AssemblyType::QUAD_WORD, src->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<JmpInstruction>(label2));
        instructions.emplace_back(std::make_unique<LabelInstruction>(label1));
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::DOUBLE, src->clone(), regx->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::SUB, AssemblyType::DOUBLE, std::make_unique<DataOperand>(upper_bound_label), regx->clone()));
        instructions.emplace_back(std::make_unique<Cvttsd2siInstruction>(AssemblyType::QUAD_WORD, regx->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::AND, AssemblyType::QUAD_WORD, std::make_unique<ImmediateValue>(9223372036854775808ul), regr->clone()));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::ADD, AssemblyType::QUAD_WORD, regr->clone(), dst->clone()));
        instructions.emplace_back(std::make_unique<LabelInstruction>(label2));
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_unary_instruction(tacky::UnaryInstruction& unary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto source_type = get_operand_type(*unary_instruction.source).first;
    auto destination_type = get_operand_type(*unary_instruction.destination).first;
    bool is_double = (source_type == AssemblyType::DOUBLE);
    std::unique_ptr<Operand> src = transform_operand(*unary_instruction.source);
    std::unique_ptr<Operand> dst = transform_operand(*unary_instruction.destination);
    std::unique_ptr<Operand> dst_copy = dst->clone();
    add_comment_instruction(std::format("unary_instruction operator: {}", static_cast<int>(unary_instruction.unary_operator)), instructions);
    if (unary_instruction.unary_operator == tacky::UnaryOperator::NOT) {
        if (is_double) {
            RegisterName reg = RegisterName::XMM0;
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::XOR, AssemblyType::DOUBLE, std::make_unique<Register>(reg), std::make_unique<Register>(reg)));
            instructions.emplace_back(std::make_unique<CmpInstruction>(source_type, std::make_unique<Register>(reg), std::move(src)));
        } else {
            instructions.emplace_back(std::make_unique<CmpInstruction>(source_type, std::make_unique<ImmediateValue>(0), std::move(src)));
        }
        instructions.emplace_back(std::make_unique<MovInstruction>(destination_type, std::make_unique<ImmediateValue>(0), std::move(dst)));
        instructions.emplace_back(std::make_unique<SetCCInstruction>(ConditionCode::E, std::move(dst_copy)));

    } else if (is_double && unary_instruction.unary_operator == tacky::UnaryOperator::NEGATE) {
        // We need to align -0.0 to 16 bytes so that we can use it in the xorpd instruction
        std::string const_label = add_static_double_constant(-0.0, 16);
        auto data_operand = std::make_unique<DataOperand>(const_label);
        instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::DOUBLE, std::move(src), std::move(dst)));
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::XOR, AssemblyType::DOUBLE, std::move(data_operand), std::move(dst_copy)));
    } else {

        instructions.emplace_back(std::make_unique<MovInstruction>(source_type, std::move(src), std::move(dst)));
        UnaryOperator op = transform_operator(unary_instruction.unary_operator);
        instructions.emplace_back(std::make_unique<UnaryInstruction>(op, source_type, std::move(dst_copy)));
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_binary_instruction(tacky::BinaryInstruction& binary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    auto [source1_type, is_signed] = get_operand_type(*binary_instruction.source1);
    bool is_double = source1_type == AssemblyType::DOUBLE;
    auto destination_type = get_operand_type(*binary_instruction.destination).first;
    if (is_relational_operator(binary_instruction.binary_operator)) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        add_comment_instruction("relational binary_instruction", instructions);
        instructions.emplace_back(std::make_unique<CmpInstruction>(source1_type, std::move(src2), std::move(src1)));
        instructions.emplace_back(std::make_unique<MovInstruction>(destination_type, std::make_unique<ImmediateValue>(0), std::move(dst)));
        // condition code differs between signed and unsigned/double
        instructions.emplace_back(std::make_unique<SetCCInstruction>(to_condition_code(binary_instruction.binary_operator, is_signed), std::move(dst_copy)));
    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::DIVIDE) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        add_comment_instruction("divide binary_instruction", instructions);
        if (is_double) {
            instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::move(dst)));
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::DIV_DOUBLE, source1_type, std::move(src2), std::move(dst_copy)));
        } else {
            instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::make_unique<Register>(RegisterName::AX)));
            if (is_signed) {
                instructions.emplace_back(std::make_unique<CdqInstruction>(source1_type));
                instructions.emplace_back(std::make_unique<IdivInstruction>(source1_type, std::move(src2)));
            } else {
                instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<ImmediateValue>(0), std::make_unique<Register>(RegisterName::DX)));
                instructions.emplace_back(std::make_unique<DivInstruction>(source1_type, std::move(src2)));
            }

            instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<Register>(RegisterName::AX), std::move(dst)));
        }

    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::REMAINDER) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        add_comment_instruction("remainder binary_instruction", instructions);
        instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::make_unique<Register>(RegisterName::AX)));
        if (is_signed) {
            instructions.emplace_back(std::make_unique<CdqInstruction>(source1_type));
            instructions.emplace_back(std::make_unique<IdivInstruction>(source1_type, std::move(src2)));
        } else {
            instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<ImmediateValue>(0), std::make_unique<Register>(RegisterName::DX)));
            instructions.emplace_back(std::make_unique<DivInstruction>(source1_type, std::move(src2)));
        }
        instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::make_unique<Register>(RegisterName::DX), std::move(dst)));
    } else {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        add_comment_instruction("arithmetic binary_instruction", instructions);
        instructions.emplace_back(std::make_unique<MovInstruction>(source1_type, std::move(src1), std::move(dst)));

        BinaryOperator op = transform_operator(binary_instruction.binary_operator);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        instructions.emplace_back(std::make_unique<BinaryInstruction>(op, source1_type, std::move(src2), std::move(dst_copy)));
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_jump_instruction(tacky::Instruction& instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    if (tacky::JumpInstruction* jump_instruction = dynamic_cast<tacky::JumpInstruction*>(&instruction)) {
        add_comment_instruction("jump_instruction", instructions);
        instructions.emplace_back(std::make_unique<JmpInstruction>(jump_instruction->identifier.name));
    } else if (tacky::JumpIfZeroInstruction* jump_if_zero_instruction = dynamic_cast<tacky::JumpIfZeroInstruction*>(&instruction)) {
        auto [condition_type, _] = get_operand_type(*jump_if_zero_instruction->condition);
        bool is_double = (condition_type == AssemblyType::DOUBLE);
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_zero_instruction->condition);
        add_comment_instruction("jump_if_zero_instruction", instructions);
        if (is_double) {
            // zero-out XMM0
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::XOR, condition_type, std::make_unique<Register>(RegisterName::XMM0), std::make_unique<Register>(RegisterName::XMM0)));
            instructions.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<Register>(RegisterName::XMM0), std::move(cond)));
        } else {
            instructions.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<ImmediateValue>(0), std::move(cond)));
        }

        instructions.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::E, jump_if_zero_instruction->identifier.name));
    } else if (tacky::JumpIfNotZeroInstruction* jump_if_not_zero_instruction = dynamic_cast<tacky::JumpIfNotZeroInstruction*>(&instruction)) {
        auto [condition_type, _] = get_operand_type(*jump_if_not_zero_instruction->condition);
        bool is_double = (condition_type == AssemblyType::DOUBLE);
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_not_zero_instruction->condition);
        add_comment_instruction("jump_if_not_zero_instruction", instructions);
        if (is_double) {
            // zero-out XMM0
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::XOR, AssemblyType::DOUBLE, std::make_unique<Register>(RegisterName::XMM0), std::make_unique<Register>(RegisterName::XMM0)));
            instructions.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<Register>(RegisterName::XMM0), std::move(cond)));
        } else {
            instructions.emplace_back(std::make_unique<CmpInstruction>(condition_type, std::make_unique<ImmediateValue>(0), std::move(cond)));
        }
        instructions.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::NE, jump_if_not_zero_instruction->identifier.name));
    } else {
        assert(false && "AssemblyGenerator::transform_jump_instruction Invalid or Unsupported tacky::Instruction");
    }
    return instructions;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_function_call_instruction(tacky::FunctionCallInstruction& function_call_instruction)
{
    std::vector<std::unique_ptr<Instruction>> instructions;

    std::vector<std::unique_ptr<tacky::Value>>& tacky_arguments = function_call_instruction.arguments;

    // classify parameters
    std::vector<size_t> int_reg_args;
    std::vector<size_t> double_reg_args;
    std::vector<size_t> stack_args;
    for (size_t i = 0; i < tacky_arguments.size(); ++i) {
        auto [arg_type, _] = get_operand_type(*tacky_arguments[i].get());
        if (arg_type == AssemblyType::DOUBLE) {
            if (double_reg_args.size() < DOUBLE_FUNCTION_REGISTERS.size()) {
                double_reg_args.push_back(i);
            } else {
                stack_args.push_back(i);
            }
        } else {
            if (int_reg_args.size() < INT_FUNCTION_REGISTERS.size()) {
                int_reg_args.push_back(i);
            } else {
                stack_args.push_back(i);
            }
        }
    }

    // adjust stack alignment
    int stack_padding = 0;
    if (stack_args.size() % 2 != 0) {
        stack_padding = 8;
    }

    if (stack_padding != 0) {
        add_comment_instruction("function_call stack padding", instructions);
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::SUB, AssemblyType::QUAD_WORD,
            std::make_unique<ImmediateValue>(stack_padding), std::make_unique<Register>(RegisterName::SP)));
    }

    if (int_reg_args.size() > 0) {
        add_comment_instruction("function_call int register arguments", instructions);
        size_t reg_offset = 0;
        for (size_t i : int_reg_args) {
            RegisterName reg_name = INT_FUNCTION_REGISTERS[reg_offset];
            auto [arg_type, _] = get_operand_type(*tacky_arguments[i].get());
            std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[i].get());

            instructions.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(assembly_arg), std::make_unique<Register>(reg_name)));
            ++reg_offset;
        }
    }

    if (double_reg_args.size() > 0) {
        add_comment_instruction("function_call double register arguments", instructions);
        size_t reg_offset = 0;
        for (size_t i : double_reg_args) {
            RegisterName reg_name = DOUBLE_FUNCTION_REGISTERS[reg_offset];
            auto [arg_type, _] = get_operand_type(*tacky_arguments[i].get());
            std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[i].get());

            instructions.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(assembly_arg), std::make_unique<Register>(reg_name)));
            ++reg_offset;
        }
    }

    if (stack_args.size() > 0) {
        add_comment_instruction("function_call stack arguments", instructions);
    }

    for (size_t i : std::views::reverse(stack_args)) {
        auto [arg_type, _] = get_operand_type(*tacky_arguments[i].get());
        std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[i].get());
        if (dynamic_cast<Register*>(assembly_arg.get()) || dynamic_cast<ImmediateValue*>(assembly_arg.get()) || arg_type == AssemblyType::QUAD_WORD || arg_type == AssemblyType::DOUBLE) {
            instructions.emplace_back(std::make_unique<PushInstruction>(std::move(assembly_arg)));
        } else {
            // Because we can run into trouble if we push a 4-byte operand from memory into the stack we first move it into AX
            instructions.emplace_back(std::make_unique<MovInstruction>(arg_type, std::move(assembly_arg), std::make_unique<Register>(RegisterName::AX)));
            instructions.emplace_back(std::make_unique<PushInstruction>(std::make_unique<Register>(RegisterName::AX)));
        }
    }

    // emit call instruciton
    instructions.emplace_back(std::make_unique<CallInstruction>(function_call_instruction.name.name));

    // adjust stack pointer
    int bytes_to_remove = 8 * stack_args.size() + stack_padding;
    if (bytes_to_remove != 0) {
        add_comment_instruction("function_call adjust stack pointer", instructions);
        instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::ADD, AssemblyType::QUAD_WORD,
            std::make_unique<ImmediateValue>(bytes_to_remove), std::make_unique<Register>(RegisterName::SP)));
    }

    // retrieve return value
    std::unique_ptr<Operand> dst = transform_operand(*function_call_instruction.destination);
    auto [dst_type, _] = get_operand_type(*function_call_instruction.destination);
    bool is_dst_double = (dst_type == AssemblyType::DOUBLE);
    add_comment_instruction("function_call mov return value", instructions);
    instructions.emplace_back(std::make_unique<MovInstruction>(dst_type, std::make_unique<Register>(is_dst_double ? RegisterName::XMM0 : RegisterName::AX), std::move(dst)));
    return instructions;
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
    const auto& param_types = function_type->parameters_type;

    // classify parameters
    std::vector<size_t> int_reg_params;
    std::vector<size_t> double_reg_params;
    std::vector<size_t> stack_params;
    for (size_t i = 0; i < tacky_parameters.size(); ++i) {
        auto [param_type, _] = convert_type(*param_types.at(i));
        if (param_type == AssemblyType::DOUBLE) {
            if (double_reg_params.size() < DOUBLE_FUNCTION_REGISTERS.size()) {
                double_reg_params.push_back(i);
            } else {
                stack_params.push_back(i);
            }
        } else {
            if (int_reg_params.size() < INT_FUNCTION_REGISTERS.size()) {
                int_reg_params.push_back(i);
            } else {
                stack_params.push_back(i);
            }
        }
    }

    // move register parameters from registers to pseudo registers
    if (int_reg_params.size() > 0) {
        add_comment_instruction("function_definition int register parameters", instructions);
        size_t reg_offset = 0;
        for (size_t i : int_reg_params) {
            auto [param_type, _] = convert_type(*param_types.at(i));

            std::unique_ptr<Register> reg = std::make_unique<Register>(INT_FUNCTION_REGISTERS[reg_offset]);
            std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
            instructions.emplace_back(std::make_unique<MovInstruction>(param_type, std::move(reg), std::move(pseudo_reg)));
            ++reg_offset;
        }
    }

    // move register parameters from registers to pseudo registers
    if (double_reg_params.size() > 0) {
        add_comment_instruction("function_definition double register parameters", instructions);
        size_t reg_offset = 0;
        for (size_t i : double_reg_params) {
            auto [param_type, _] = convert_type(*param_types.at(i));

            std::unique_ptr<Register> reg = std::make_unique<Register>(DOUBLE_FUNCTION_REGISTERS[reg_offset]);
            std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
            instructions.emplace_back(std::make_unique<MovInstruction>(param_type, std::move(reg), std::move(pseudo_reg)));
            ++reg_offset;
        }
    }

    if (stack_params.size() > 0) {
        add_comment_instruction("function_definition stack parameters", instructions);
    }

    int stack_offset = 16;
    for (size_t i : stack_params) {
        auto [param_type, _] = convert_type(*param_types.at(i));
        auto stack_addr = std::make_unique<MemoryAddress>(RegisterName::BP, stack_offset);
        std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
        instructions.emplace_back(std::make_unique<MovInstruction>(param_type, std::move(stack_addr), std::move(pseudo_reg)));
        stack_offset += 8;
    }

    add_comment_instruction("function_definition body", instructions);
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
        assert(false && "In transform_top_level: invalid top level class");
        return nullptr;
    }
}

std::unique_ptr<Program> AssemblyGenerator::transform_program(tacky::Program& program)
{
    std::vector<std::unique_ptr<TopLevel>> definitions;

    for (auto& def : program.definitions) {
        definitions.emplace_back(transform_top_level(*def));
    }

    // Add constants to the list of top level constructs
    for (auto& p : m_static_constants_map) {
        definitions.emplace_back(std::move(p.second.second));
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

ConditionCode AssemblyGenerator::to_condition_code(tacky::BinaryOperator op, bool is_signed)
{
    switch (op) {
    case tacky::BinaryOperator::EQUAL:
        return ConditionCode::E;
    case tacky::BinaryOperator::NOT_EQUAL:
        return ConditionCode::NE;
    case tacky::BinaryOperator::LESS_THAN:
        return is_signed ? ConditionCode::L : ConditionCode::B;
    case tacky::BinaryOperator::LESS_OR_EQUAL:
        return is_signed ? ConditionCode::LE : ConditionCode::BE;
    case tacky::BinaryOperator::GREATER_THAN:
        return is_signed ? ConditionCode::G : ConditionCode::A;
    case tacky::BinaryOperator::GREATER_OR_EQUAL:
        return is_signed ? ConditionCode::GE : ConditionCode::AE;
    default:
        assert(false && "AssemblyGenerator::to_condition_code is not a relational operator");
        return ConditionCode::NONE;
    }
}

std::pair<AssemblyType, bool> AssemblyGenerator::get_operand_type(tacky::Value& operand)
{
    AssemblyType type;
    bool is_signed = false;
    if (tacky::Constant* tacky_constant = dynamic_cast<tacky::Constant*>(&operand)) {
        if (std::holds_alternative<int>(tacky_constant->value)) {
            type = AssemblyType::LONG_WORD;
            is_signed = true;
        } else if (std::holds_alternative<long>(tacky_constant->value)) {
            type = AssemblyType::QUAD_WORD;
            is_signed = true;
        } else if (std::holds_alternative<unsigned int>(tacky_constant->value)) {
            type = AssemblyType::LONG_WORD;
            is_signed = false;
        } else if (std::holds_alternative<unsigned long>(tacky_constant->value)) {
            type = AssemblyType::QUAD_WORD;
            is_signed = false;
        } else if (std::holds_alternative<double>(tacky_constant->value)) {
            type = AssemblyType::DOUBLE;
            is_signed = false;
        }
    } else if (tacky::TemporaryVariable* tacky_var = dynamic_cast<tacky::TemporaryVariable*>(&operand)) {
        const auto& symbol = m_symbol_table->symbol_at(tacky_var->identifier.name);
        std::tie(type, is_signed) = convert_type(*symbol.type);
    }

    return { type, is_signed };
}

std::pair<AssemblyType, bool> AssemblyGenerator::convert_type(const Type& type)
{
    AssemblyType assembly_type;
    bool is_signed = type.is_signed();
    if (dynamic_cast<const IntType*>(&type)) {
        assembly_type = AssemblyType::LONG_WORD;
    } else if (dynamic_cast<const LongType*>(&type)) {
        assembly_type = AssemblyType::QUAD_WORD;
    } else if (dynamic_cast<const UnsignedIntType*>(&type)) {
        assembly_type = AssemblyType::LONG_WORD;
    } else if (dynamic_cast<const UnsignedLongType*>(&type)) {
        assembly_type = AssemblyType::QUAD_WORD;
    } else if (dynamic_cast<const DoubleType*>(&type)) {
        assembly_type = AssemblyType::DOUBLE;
    } else if (dynamic_cast<const PointerType*>(&type)) {
        assembly_type = AssemblyType::QUAD_WORD;
    } else if (auto arr_type = dynamic_cast<const ArrayType*>(&type)) {
        // for variables less than 16 bytes use same alignement as element
        size_t alignment = (arr_type->size() >= 16) ? 16 : arr_type->alignment();
        assembly_type = AssemblyType(AssemblyType::BYTE_ARRAY, arr_type->size(), alignment);
    }
    return { assembly_type, is_signed };
}

void AssemblyGenerator::add_comment_instruction(const std::string& message, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    if (m_compile_options->enable_assembly_comments) {
        instructions.emplace_back(std::make_unique<CommentInstruction>(message));
    }
}

std::string AssemblyGenerator::add_static_double_constant(double val, size_t alignment)
{
    std::string label = get_constant_label(val, alignment);
    if (!m_static_constants_map.contains(label)) {
        std::string compact_label = "const_label_" + std::to_string(m_static_constants_map.size());
        m_static_constants_map[label].first = compact_label;
        StaticInitialValue init;
        init.values = { StaticInitialValueType(val) };
        m_static_constants_map[label].second = std::make_unique<StaticConstant>(compact_label, alignment, init);
        m_backend_symbol_table->insert_symbol(compact_label, ObjectEntry { AssemblyType::DOUBLE, true, true });
    }
    return m_static_constants_map[label].first;
}

std::string AssemblyGenerator::get_constant_label(double val, size_t alignment)
{
    // This prevent duplicate constants
    return std::format("{}_{}", val, alignment);
}
