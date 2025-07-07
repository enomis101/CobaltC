#pragma once
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace backend {

class FixUpInstructionsStepError : public std::runtime_error {
public:
    explicit FixUpInstructionsStepError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class FixUpInstructionsStep : public AssemblyVisitor {
public:
    FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table);

    void fixup();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(MemoryAddress& node) override { }
    void visit(DataOperand& node) override { }
    void visit(CommentInstruction& node) override { } // NOT NEEDED
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override { }
    void visit(MovsxInstruction& node) override { }
    void visit(MovZeroExtendInstruction& node) override { }
    void visit(LeaInstruction& node) override { }
    void visit(Cvttsd2siInstruction& node) override { }
    void visit(Cvtsi2sdInstruction& node) override { }
    void visit(UnaryInstruction& node) override { }
    void visit(BinaryInstruction& node) override { }
    void visit(CmpInstruction& node) override { }
    void visit(IdivInstruction& node) override { }
    void visit(DivInstruction& node) override { }
    void visit(CdqInstruction& node) override { }
    void visit(JmpInstruction& node) override { }
    void visit(JmpCCInstruction& node) override { }
    void visit(SetCCInstruction& node) override { }
    void visit(LabelInstruction& node) override { }
    void visit(PushInstruction& node) override { }
    void visit(CallInstruction& node) override { }
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override { }
    void visit(StaticConstant& node) override { }
    void visit(Program& node) override;

    void fixup_instructions(std::vector<std::unique_ptr<Instruction>>& old_instructions, std::vector<std::unique_ptr<Instruction>>& new_instructions);

    void fixup_mov_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_cmp_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_binary_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_idiv_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_div_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_movsx_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_mov_zero_extend_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_lea_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_push_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_cvttsd2si_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);
    void fixup_cvtsi2sd_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions);

    bool is_xmm_register(RegisterName reg);

    std::shared_ptr<AssemblyAST> m_ast;
    std::shared_ptr<BackendSymbolTable> m_symbol_table;

    size_t round_up_to_16(size_t x)
    {
        return ((x + 15) / 16) * 16;
    }
};

} // namespace backend
