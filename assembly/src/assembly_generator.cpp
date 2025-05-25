#include "assembly/assembly_generator.h"
#include "assembly/assembly_ast.h"

#include "parser/symbol_table.h"
#include "tacky/tacky_ast.h"
#include <cassert>
#include <memory>
#include <string>
#include <variant>

using namespace assembly;

PseudoRegisterReplaceStep::PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast)
    : m_ast { ast }
    , s_symbol_table { parser::SymbolTable::instance() }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("PseudoRegisterReplaceStep: Invalid AST");
    }
}

void PseudoRegisterReplaceStep::replace()
{
    m_ast->accept(*this);
}

void PseudoRegisterReplaceStep::visit(MovInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(UnaryInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(BinaryInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(CmpInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(SetCCInstruction& node)
{
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(IdivInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(PushInstruction& node)
{
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(FunctionDefinition& node)
{
    m_stack_offsets.clear();

    for (auto& i : node.instructions) {
        i->accept(*this);
    }

    size_t stack_size = m_stack_offsets.size() * 4;
    s_symbol_table.symbols().at(node.name.name).stack_size = stack_size;
}

void PseudoRegisterReplaceStep::visit(Program& node)
{
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}

void PseudoRegisterReplaceStep::check_and_replace(std::unique_ptr<Operand>& op)
{
    if (PseudoRegister* reg = dynamic_cast<PseudoRegister*>(op.get())) {
        const std::string& pseudo_reg_name = reg->identifier.name;
        std::unique_ptr<Operand> new_op = nullptr;
        if (!m_stack_offsets.contains(pseudo_reg_name) &&
         s_symbol_table.symbols().contains(pseudo_reg_name) && std::holds_alternative<parser::StaticAttribute>(s_symbol_table.symbols().at(pseudo_reg_name).attribute)) {
            new_op = std::make_unique<DataOperand>(pseudo_reg_name);
        } else {
            int offset = get_offset(pseudo_reg_name);
            new_op = std::make_unique<StackAddress>(offset);
        }
        op = std::move(new_op);
    }
}

int PseudoRegisterReplaceStep::get_offset(const std::string& name)
{
    if (!m_stack_offsets.contains(name)) {
        m_stack_offsets[name] = m_stack_offsets.size() + 1;
    }

    return m_stack_offsets[name] * -4;
}

FixUpInstructionsStep::FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast)
    : m_ast { ast }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("FixUpInstructionsStep: Invalid AST");
    }
}

void FixUpInstructionsStep::fixup()
{
    m_ast->accept(*this);
}

void FixUpInstructionsStep::visit(FunctionDefinition& node)
{
    std::vector<std::unique_ptr<Instruction>> tmp_instructions = std::move(node.instructions);
    node.instructions.clear();

    parser::SymbolTable& symbol_table = parser::SymbolTable::instance();
    int stack_offset = round_up_to_16(symbol_table.symbols().at(node.name.name).stack_size);
    node.instructions.emplace_back(std::make_unique<AllocateStackInstruction>(stack_offset));
    for (auto& i : tmp_instructions) {
        if (dynamic_cast<MovInstruction*>(i.get())) {
            fixup_double_memory_address_instruction<MovInstruction>(i, node.instructions);
        } else if (CmpInstruction* cmp_instruction = dynamic_cast<CmpInstruction*>(i.get())) {
            if (dynamic_cast<ImmediateValue*>(cmp_instruction->destination.get())) {
                // destination of cmp cant be a constant
                node.instructions.emplace_back(std::make_unique<MovInstruction>(std::move(cmp_instruction->destination), std::make_unique<Register>(RegisterName::R11)));
                cmp_instruction->destination = std::make_unique<Register>(RegisterName::R11);
                node.instructions.emplace_back(std::move(i));
            } else {
                fixup_double_memory_address_instruction<CmpInstruction>(i, node.instructions);
            }

        } else if (BinaryInstruction* binary_instruction = dynamic_cast<BinaryInstruction*>(i.get())) {
            if ((binary_instruction->binary_operator == BinaryOperator::ADD) || (binary_instruction->binary_operator == BinaryOperator::SUB)) {
                fixup_double_memory_address_instruction<BinaryInstruction>(i, node.instructions);
            } else if (binary_instruction->binary_operator == BinaryOperator::MULT) {
                // imul cant use memory addresses as its destination
                std::unique_ptr<Operand> destination_copy = binary_instruction->destination->clone();
                // Load destination into R11 register
                node.instructions.emplace_back(std::make_unique<MovInstruction>(std::move(binary_instruction->destination), std::make_unique<Register>(RegisterName::R11)));
                binary_instruction->destination = std::make_unique<Register>(RegisterName::R11);
                node.instructions.emplace_back(std::move(i));
                node.instructions.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::R11), std::move(destination_copy)));
            } else {
                node.instructions.push_back(std::move(i));
            }
        } else if (IdivInstruction* div_instruction = dynamic_cast<IdivInstruction*>(i.get())) {
            // idiv cant operate on immediate values:
            if (dynamic_cast<ImmediateValue*>(div_instruction->operand.get())) {
                node.instructions.emplace_back(std::make_unique<MovInstruction>(std::move(div_instruction->operand), std::make_unique<Register>(RegisterName::R10)));
                div_instruction->operand = std::make_unique<Register>(RegisterName::R10);
            }
            node.instructions.emplace_back(std::move(i));
        } else {
            node.instructions.emplace_back(std::move(i));
        }
    }
}

void FixUpInstructionsStep::visit(Program& node)
{
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast)
    : m_ast { ast }
    , FUN_REGISTERS { RegisterName::DI, RegisterName::SI, RegisterName::DX, RegisterName::CX, RegisterName::R8, RegisterName::R9 }
{
    if (!m_ast || !dynamic_cast<tacky::Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid AST");
    }
}

std::shared_ptr<AssemblyAST> AssemblyGenerator::generate()
{
    std::shared_ptr<AssemblyAST> m_assembly_ast = transform_program(*dynamic_cast<tacky::Program*>(m_ast.get()));

    PseudoRegisterReplaceStep step1(m_assembly_ast);
    step1.replace();
    FixUpInstructionsStep step2(m_assembly_ast);
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
    if (it != unary_op_map.end()) {
        return it->second;
    }
    throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::UnaryOperator");
}

BinaryOperator AssemblyGenerator::transform_operator(tacky::BinaryOperator& binary_operator)
{
    static const std::unordered_map<tacky::BinaryOperator, BinaryOperator> binary_op_map = {
        { tacky::BinaryOperator::ADD, BinaryOperator::ADD },
        { tacky::BinaryOperator::SUBTRACT, BinaryOperator::SUB },
        { tacky::BinaryOperator::MULTIPLY, BinaryOperator::MULT }
    };

    auto it = binary_op_map.find(binary_operator);
    if (it != binary_op_map.end()) {
        return it->second;
    }

    throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::BinaryOperator");
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_instruction(tacky::Instruction& instruction)
{
    if (tacky::ReturnInstruction* return_instruction = dynamic_cast<tacky::ReturnInstruction*>(&instruction)) {
        std::vector<std::unique_ptr<Instruction>> res;
        std::unique_ptr<Operand> src = transform_operand(*return_instruction->value.get());
        std::unique_ptr<Operand> dst = std::make_unique<Register>(RegisterName::AX);
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));
        res.emplace_back(std::make_unique<ReturnInstruction>());
        return res;
    } else if (tacky::UnaryInstruction* unary_instruction = dynamic_cast<tacky::UnaryInstruction*>(&instruction)) {
        return transform_unary_instruction(*unary_instruction);
    } else if (tacky::BinaryInstruction* binary_instruction = dynamic_cast<tacky::BinaryInstruction*>(&instruction)) {
        return transform_binary_instruction(*binary_instruction);
    } else if (dynamic_cast<tacky::JumpInstruction*>(&instruction) || dynamic_cast<tacky::JumpIfZeroInstruction*>(&instruction) || dynamic_cast<tacky::JumpIfNotZeroInstruction*>(&instruction)) {
        return transform_jump_instruction(instruction);
    } else if (tacky::CopyInstruction* copy_instruction = dynamic_cast<tacky::CopyInstruction*>(&instruction)) {
        std::vector<std::unique_ptr<Instruction>> res;
        std::unique_ptr<Operand> src = transform_operand(*copy_instruction->source);
        std::unique_ptr<Operand> dst = transform_operand(*copy_instruction->destination);
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));
        return res;
    } else if (tacky::LabelInstruction* label_instruction = dynamic_cast<tacky::LabelInstruction*>(&instruction)) {
        std::vector<std::unique_ptr<Instruction>> res;
        res.emplace_back(std::make_unique<LabelInstruction>(label_instruction->identifier.name));
        return res;
    } else if (tacky::FunctionCallInstruction* function_call_instruction = dynamic_cast<tacky::FunctionCallInstruction*>(&instruction)) {
        return transform_function_call_instruction(*function_call_instruction);
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::Instruction");
    }
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_unary_instruction(tacky::UnaryInstruction& unary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    if (unary_instruction.unary_operator == tacky::UnaryOperator::NOT) {
        std::unique_ptr<Operand> src = transform_operand(*unary_instruction.source);
        std::unique_ptr<Operand> dst = transform_operand(*unary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<CmpInstruction>(std::make_unique<ImmediateValue>(0), std::move(src)));
        res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<ImmediateValue>(0), std::move(dst)));
        res.emplace_back(std::make_unique<SetCCInstruction>(ConditionCode::E, std::move(dst_copy)));
    } else {
        std::unique_ptr<Operand> src = transform_operand(*unary_instruction.source);
        std::unique_ptr<Operand> dst = transform_operand(*unary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));

        UnaryOperator op = transform_operator(unary_instruction.unary_operator);
        res.emplace_back(std::make_unique<UnaryInstruction>(op, std::move(dst_copy)));
    }
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_binary_instruction(tacky::BinaryInstruction& binary_instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    if (is_relational_operator(binary_instruction.binary_operator)) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<CmpInstruction>(std::move(src2), std::move(src1)));
        res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<ImmediateValue>(0), std::move(dst)));
        res.emplace_back(std::make_unique<SetCCInstruction>(to_condition_code(binary_instruction.binary_operator), std::move(dst_copy)));
    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::DIVIDE) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::make_unique<Register>(RegisterName::AX)));
        res.emplace_back(std::make_unique<CdqInstruction>());
        res.emplace_back(std::make_unique<IdivInstruction>(std::move(src2)));
        res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::AX), std::move(dst)));
    } else if (binary_instruction.binary_operator == tacky::BinaryOperator::REMAINDER) {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::make_unique<Register>(RegisterName::AX)));
        res.emplace_back(std::make_unique<CdqInstruction>());
        res.emplace_back(std::make_unique<IdivInstruction>(std::move(src2)));
        res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::DX), std::move(dst)));
    } else {
        std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction.source1);
        std::unique_ptr<Operand> dst = transform_operand(*binary_instruction.destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::move(dst)));

        BinaryOperator op = transform_operator(binary_instruction.binary_operator);
        std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction.source2);
        res.emplace_back(std::make_unique<BinaryInstruction>(op, std::move(src2), std::move(dst_copy)));
    }
    return res;
}

std::vector<std::unique_ptr<Instruction>> AssemblyGenerator::transform_jump_instruction(tacky::Instruction& instruction)
{
    std::vector<std::unique_ptr<Instruction>> res;
    if (tacky::JumpInstruction* jump_instruction = dynamic_cast<tacky::JumpInstruction*>(&instruction)) {
        res.emplace_back(std::make_unique<JmpInstruction>(jump_instruction->identifier.name));
    } else if (tacky::JumpIfZeroInstruction* jump_if_zero_instruction = dynamic_cast<tacky::JumpIfZeroInstruction*>(&instruction)) {
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_zero_instruction->condition);
        res.emplace_back(std::make_unique<CmpInstruction>(std::make_unique<ImmediateValue>(0), std::move(cond)));
        res.emplace_back(std::make_unique<JmpCCInstruction>(ConditionCode::E, jump_if_zero_instruction->identifier.name));
    } else if (tacky::JumpIfNotZeroInstruction* jump_if_not_zero_instruction = dynamic_cast<tacky::JumpIfNotZeroInstruction*>(&instruction)) {
        std::unique_ptr<Operand> cond = transform_operand(*jump_if_not_zero_instruction->condition);
        res.emplace_back(std::make_unique<CmpInstruction>(std::make_unique<ImmediateValue>(0), std::move(cond)));
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
        res.emplace_back(std::make_unique<AllocateStackInstruction>(stack_padding));
    }

    for (size_t i = 0; i < reg_args_size; ++i) {
        RegisterName reg_name = FUN_REGISTERS[i];
        std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[i].get());
        res.emplace_back(std::make_unique<MovInstruction>(std::move(assembly_arg), std::make_unique<Register>(reg_name)));
    }

    for (size_t i = 0; i < stack_args_size; ++i) {
        int j = tacky_arguments.size() - 1 - i; // reverse
        std::unique_ptr<Operand> assembly_arg = transform_operand(*tacky_arguments[j].get());
        if (dynamic_cast<Register*>(assembly_arg.get()) || dynamic_cast<ImmediateValue*>(assembly_arg.get())) {
            res.emplace_back(std::make_unique<PushInstruction>(std::move(assembly_arg)));
        } else {
            res.emplace_back(std::make_unique<MovInstruction>(std::move(assembly_arg), std::make_unique<Register>(RegisterName::AX)));
            res.emplace_back(std::make_unique<PushInstruction>(std::make_unique<Register>(RegisterName::AX)));
        }
    }

    // emit call instruciton
    res.emplace_back(std::make_unique<CallInstruction>(function_call_instruction.name.name));

    // adjust stack pointer
    int bytes_to_remove = 8 * stack_args_size + stack_padding;
    if (bytes_to_remove != 0) {
        res.emplace_back(std::make_unique<DeallocateStackInstruction>(bytes_to_remove));
    }

    // retrieve return value
    std::unique_ptr<Operand> dst = transform_operand(*function_call_instruction.destination);
    res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::AX), std::move(dst)));
    return res;
}

std::unique_ptr<FunctionDefinition> AssemblyGenerator::transform_function(tacky::FunctionDefinition& function_definition)
{
    std::vector<std::unique_ptr<Instruction>> instructions;

    std::vector<std::string> tacky_parameters;
    for (auto& par : function_definition.parameters) {
        tacky_parameters.emplace_back(par.name);
    }

    const size_t max_reg_args_size = FUN_REGISTERS.size();
    const size_t reg_args_size = std::min(max_reg_args_size, tacky_parameters.size());

    // move register parameters from registers to pseudo registers
    for (size_t i = 0; i < reg_args_size; ++i) {
        std::unique_ptr<Register> reg = std::make_unique<Register>(FUN_REGISTERS[i]);
        std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
        instructions.emplace_back(std::make_unique<MovInstruction>(std::move(reg), std::move(pseudo_reg)));
    }

    int stack_offset = 16;
    for (size_t i = reg_args_size; i < tacky_parameters.size(); ++i, stack_offset += 8) {
        std::unique_ptr<StackAddress> stack_addr = std::make_unique<StackAddress>(stack_offset);
        std::unique_ptr<PseudoRegister> pseudo_reg = std::make_unique<PseudoRegister>(tacky_parameters[i]);
        instructions.emplace_back(std::make_unique<MovInstruction>(std::move(stack_addr), std::move(pseudo_reg)));
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
        return std::make_unique<StaticVariable>(static_var->name.name, static_var->global, static_var->init);
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
