#include "assembly/assembly_generator.h"
#include <cassert>
#include <fstream>
#include <string>

using namespace assembly;

PseudoRegisterReplaceStep::PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast)
    : m_ast{ast}
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw AssemblyGeneratorError("PseudoRegisterReplaceStep: Invalid AST");
    }
}

void PseudoRegisterReplaceStep::replace()
{
    m_stack_offsets.clear();
}

void PseudoRegisterReplaceStep::visit(Identifier& node)
{

}

void PseudoRegisterReplaceStep::visit(ImmediateValue& node)
{

}

void PseudoRegisterReplaceStep::visit(Register& node)
{

}

void PseudoRegisterReplaceStep::visit(PseudoRegister& node)
{

}

void PseudoRegisterReplaceStep::visit(StackAddress& node)
{

}

void PseudoRegisterReplaceStep::visit(NotOperator& node)
{

}

void PseudoRegisterReplaceStep::visit(NegOperator& node)
{

}

void PseudoRegisterReplaceStep::visit(ReturnInstruction& node)
{

}

void PseudoRegisterReplaceStep::visit(MovInstruction& node)
{

}

void PseudoRegisterReplaceStep::visit(UnaryInstruction& node)
{

}

void PseudoRegisterReplaceStep::visit(AllocateStackInstruction& node)
{

}

void PseudoRegisterReplaceStep::visit(Function& node)
{

}

void PseudoRegisterReplaceStep::visit(Program& node)
{

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
    std::shared_ptr<AssemblyAST> m_stage1_ast = transform_program(*dynamic_cast<tacky::Program*>(m_ast.get()));
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
    if (dynamic_cast<tacky::NegateOperator*>(&op)) {
        return std::make_unique<NegOperator>();
    } else if (dynamic_cast<tacky::ComplementOperator*>(&op)) {
        return std::make_unique<NotOperator>();
    } else {
        throw AssemblyGeneratorError("AssemblyGenerator: Invalid or Unsupported tacky::UnaryOperator");
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
        res.emplace_back(std::make_unique<MovInstruction>(std::move(src), std::move(dst)));

        std::unique_ptr<UnaryOperator> op = transform_operator(*unary_instruction->unary_operator);
        std::unique_ptr<Operand> dst_copy = transform_operand(*unary_instruction->destination);
        res.emplace_back(std::make_unique<UnaryInstruction>(std::move(op), std::move(dst_copy)));

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
