#include "assembly/assembly_generator.h"
#include <cassert>
#include <fstream>
#include <string>

using namespace assembly;

PseudoRegisterReplaceStep::PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast)
    : m_ast { ast }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("PseudoRegisterReplaceStep: Invalid AST");
    }
}

int PseudoRegisterReplaceStep::replace()
{
    m_stack_offsets.clear();
    m_ast->accept(*this);
    return m_stack_offsets.size() * -4;
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

void PseudoRegisterReplaceStep::visit(IdivInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(Function& node)
{
    for (auto& i : node.instructions) {
        i->accept(*this);
    }
}

void PseudoRegisterReplaceStep::visit(Program& node)
{
    node.function->accept(*this);
}

void PseudoRegisterReplaceStep::check_and_replace(std::unique_ptr<Operand>& op)
{
    if (PseudoRegister* reg = dynamic_cast<PseudoRegister*>(op.get())) {
        int offset = get_offset(reg->identifier.name);

        std::unique_ptr<Operand> stack = std::make_unique<StackAddress>(offset);
        op = std::move(stack);
    }
}

int PseudoRegisterReplaceStep::get_offset(const std::string& name)
{
    if (!m_stack_offsets.contains(name)) {
        m_stack_offsets[name] = m_stack_offsets.size() + 1;
    }

    return m_stack_offsets[name] * -4;
}

FixUpInstructionsStep::FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast, int stack_offset)
    : m_ast { ast }
    , m_stack_offset { stack_offset }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("FixUpInstructionsStep: Invalid AST");
    }
}

void FixUpInstructionsStep::fixup()
{
    m_ast->accept(*this);
}

void FixUpInstructionsStep::visit(Function& node)
{
    std::vector<std::unique_ptr<Instruction>> tmp_instructions = std::move(node.instructions);
    node.instructions.clear();

    node.instructions.emplace_back(std::make_unique<AllocateStackInstruction>(m_stack_offset));
    for (auto& i : tmp_instructions) {
        if (dynamic_cast<MovInstruction*>(i.get())) {
            fixup_double_stack_address_instruction<MovInstruction>(i, node.instructions);
        } else if (BinaryInstruction* binary_instruction = dynamic_cast<BinaryInstruction*>(i.get())) {
            if (dynamic_cast<AddOperator*>(binary_instruction->binary_operator.get()) || dynamic_cast<SubOperator*>(binary_instruction->binary_operator.get())) {
                fixup_double_stack_address_instruction<BinaryInstruction>(i, node.instructions);
            } else if (dynamic_cast<MultOperator*>(binary_instruction->binary_operator.get())) {
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
    node.function->accept(*this);
}

AssemblyGenerator::AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast)
    : m_ast { ast }
{
    if (!m_ast || !dynamic_cast<tacky::Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid AST");
    }
}

std::shared_ptr<AssemblyAST> AssemblyGenerator::generate()
{
    std::shared_ptr<AssemblyAST> m_assembly_ast = transform_program(*dynamic_cast<tacky::Program*>(m_ast.get()));

    PseudoRegisterReplaceStep step1(m_assembly_ast);
    int offset = step1.replace();
    FixUpInstructionsStep step2(m_assembly_ast, offset);
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

std::unique_ptr<UnaryOperator> AssemblyGenerator::transform_operator(tacky::UnaryOperator& op)
{
    if (op == tacky::UnaryOperator::NEGATE) {
        return std::make_unique<NegOperator>();
    } else if (op == tacky::UnaryOperator::COMPLEMENT) {
        return std::make_unique<NotOperator>();
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::UnaryOperator");
    }
}

std::unique_ptr<BinaryOperator> AssemblyGenerator::transform_operator(tacky::BinaryOperator& op)
{
    if (op == tacky::BinaryOperator::ADD) {
        return std::make_unique<AddOperator>();
    } else if (op == tacky::BinaryOperator::SUBTRACT) {
        return std::make_unique<SubOperator>();
    } else if (op == tacky::BinaryOperator::MULTIPLY) {
        return std::make_unique<MultOperator>();
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::BinaryOperator");
    }
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
        std::vector<std::unique_ptr<Instruction>> res;

        std::unique_ptr<Operand> src = transform_operand(*unary_instruction->source);
        std::unique_ptr<Operand> dst = transform_operand(*unary_instruction->destination);
        std::unique_ptr<Operand> dst_copy = dst->clone();
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));

        std::unique_ptr<UnaryOperator> op = transform_operator(unary_instruction->unary_operator);
        res.emplace_back(std::make_unique<UnaryInstruction>(std::move(op), std::move(dst_copy)));

        return res;
    } else if (tacky::BinaryInstruction* binary_instruction = dynamic_cast<tacky::BinaryInstruction*>(&instruction)) {
        std::vector<std::unique_ptr<Instruction>> res;

        if (binary_instruction->binary_operator == tacky::BinaryOperator::DIVIDE) {
            std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction->source1);
            std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction->source2);
            std::unique_ptr<Operand> dst = transform_operand(*binary_instruction->destination);
            res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::make_unique<Register>(RegisterName::AX)));
            res.emplace_back(std::make_unique<CdqInstruction>());
            res.emplace_back(std::make_unique<IdivInstruction>(std::move(src2)));
            res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::AX), std::move(dst)));
        } else if (binary_instruction->binary_operator == tacky::BinaryOperator::REMAINDER) {
            std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction->source1);
            std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction->source2);
            std::unique_ptr<Operand> dst = transform_operand(*binary_instruction->destination);
            res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::make_unique<Register>(RegisterName::AX)));
            res.emplace_back(std::make_unique<CdqInstruction>());
            res.emplace_back(std::make_unique<IdivInstruction>(std::move(src2)));
            res.emplace_back(std::make_unique<MovInstruction>(std::make_unique<Register>(RegisterName::DX), std::move(dst)));
        } else {
            std::unique_ptr<Operand> src1 = transform_operand(*binary_instruction->source1);
            std::unique_ptr<Operand> dst = transform_operand(*binary_instruction->destination);
            std::unique_ptr<Operand> dst_copy = dst->clone();
            res.emplace_back(std::make_unique<MovInstruction>(std::move(src1), std::move(dst)));

            std::unique_ptr<BinaryOperator> op = transform_operator(binary_instruction->binary_operator);
            std::unique_ptr<Operand> src2 = transform_operand(*binary_instruction->source2);
            res.emplace_back(std::make_unique<BinaryInstruction>(std::move(op), std::move(src2), std::move(dst_copy)));
        }
        return res;
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::Instruction");
    }
}

std::unique_ptr<Function> AssemblyGenerator::transform_function(tacky::Function& function)
{
    std::vector<std::unique_ptr<Instruction>> instructions;
    for (auto& i : function.body) {
        std::vector<std::unique_ptr<Instruction>> tmp_instrucitons = transform_instruction(*i);
        for (auto& tmp_i : tmp_instrucitons) {
            instructions.push_back(std::move(tmp_i));
        }
    }
    return std::make_unique<Function>(function.name.name, std::move(instructions));
}

std::unique_ptr<Program> AssemblyGenerator::transform_program(tacky::Program& program)
{
    return std::make_unique<Program>(transform_function(*program.function));
}
