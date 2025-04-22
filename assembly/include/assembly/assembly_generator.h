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
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
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
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override { }
    void visit(UnaryInstruction& node) override { }
    void visit(AllocateStackInstruction& node) override { }
    void visit(Function& node) override;
    void visit(Program& node) override;

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
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::unique_ptr<Function> transform_function(tacky::Function& function);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    std::shared_ptr<tacky::TackyAST> m_ast;
};

} // namespace assembly
