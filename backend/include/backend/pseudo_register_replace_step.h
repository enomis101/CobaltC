#pragma once
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include <memory>
#include <stdexcept>
#include <unordered_map>

namespace backend {

class PseudoRegisterReplaceStepError : public std::runtime_error {
public:
    explicit PseudoRegisterReplaceStepError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class PseudoRegisterReplaceStep : public AssemblyVisitor {
public:
    PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table);

    void replace();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override { }
    void visit(DataOperand& node) override { }
    void visit(CommentInstruction& node) override { } // NOT NEEDED
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override;
    void visit(MovsxInstruction& node) override;
    void visit(MovZeroExtendInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(CmpInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(DivInstruction& node) override;
    void visit(CdqInstruction& node) override { }
    void visit(JmpInstruction& node) override { }
    void visit(JmpCCInstruction& node) override { }
    void visit(SetCCInstruction& node) override;
    void visit(LabelInstruction& node) override { }
    void visit(PushInstruction& node) override;
    void visit(CallInstruction& node) override { }
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override { }
    void visit(Program& node) override;

    size_t get_offset(AssemblyType type, const std::string& name);
    void check_and_replace(std::unique_ptr<Operand>& op);

    std::shared_ptr<AssemblyAST> m_ast;
    std::unordered_map<std::string, int> m_stack_offsets;
    std::shared_ptr<BackendSymbolTable> m_symbol_table;
    size_t m_curr_offset;

    // round-up to next multiple of 8
    size_t round_up_8(size_t value)
    {
        return (value + 7) & ~7;
    }
};

} // namespace backend
