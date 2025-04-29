#pragma once
#include "assembly/assembly_ast.h"
#include "tacky/tacky_ast.h"
#include <stdexcept>
#include <unordered_map>

namespace assembly {

class AssemblyGeneratorError : public std::runtime_error {
public:
    explicit AssemblyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class PseudoRegisterReplaceStep : public AssemblyVisitor {
public:
    PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast);

    int replace();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override { }
    void visit(NotOperator& node) override { }
    void visit(NegOperator& node) override { }
    void visit(AddOperator& node) override { }
    void visit(SubOperator& node) override { }
    void visit(MultOperator& node) override { }
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(CdqInstruction& node) override { }
    void visit(AllocateStackInstruction& node) override { }
    void visit(Function& node) override;
    void visit(Program& node) override;

    int get_offset(const std::string& name);
    void check_and_replace(std::unique_ptr<Operand>& op);

    std::shared_ptr<AssemblyAST> m_ast;
    std::unordered_map<std::string, int> m_stack_offsets;
};

class FixUpInstructionsStep : public AssemblyVisitor {
public:
    FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast, int stack_offset);

    void fixup();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override { }
    void visit(NotOperator& node) override { }
    void visit(NegOperator& node) override { }
    void visit(AddOperator& node) override { }
    void visit(SubOperator& node) override { }
    void visit(MultOperator& node) override { }
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override { }
    void visit(UnaryInstruction& node) override { }
    void visit(BinaryInstruction& node) override { }
    void visit(IdivInstruction& node) override { }
    void visit(CdqInstruction& node) override { }
    void visit(AllocateStackInstruction& node) override { }
    void visit(Function& node) override;
    void visit(Program& node) override;

    template<typename I>
    void fixup_double_stack_address_instruction(std::unique_ptr<Instruction>& i, std::vector<std::unique_ptr<Instruction>>& instructions)
    {
        if (I* instruction = dynamic_cast<I*>(i.get())) {
            StackAddress* source = dynamic_cast<StackAddress*>(instruction->source.get());
            StackAddress* destination = dynamic_cast<StackAddress*>(instruction->destination.get());
            if (source && destination) {
                instructions.emplace_back(std::make_unique<MovInstruction>(std::move(instruction->source), std::make_unique<Register>(RegisterName::R10)));
                instruction->source = std::make_unique<Register>(RegisterName::R10);
            }
            // push a copy of instruction
            instructions.emplace_back(std::move(i));
        } else {
            throw AssemblyGeneratorError("Invalid Instruction type passed to fixup_double_stack_address_instruction");
        }
    }

    std::shared_ptr<AssemblyAST> m_ast;
    int m_stack_offset;
};

// Generate an AssemblyAST from a TackyAST
class AssemblyGenerator {
public:
    AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast);

    std::shared_ptr<AssemblyAST> generate();

private:
    std::unique_ptr<Operand> transform_operand(tacky::Value& op);
    std::unique_ptr<UnaryOperator> transform_operator(tacky::UnaryOperator& op);
    std::unique_ptr<BinaryOperator> transform_operator(tacky::BinaryOperator& op);
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::unique_ptr<Function> transform_function(tacky::Function& function);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    std::shared_ptr<tacky::TackyAST> m_ast;
};

} // namespace assembly
