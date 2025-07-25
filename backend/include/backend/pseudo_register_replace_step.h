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
    void visit(MemoryAddress& node) override { }
    void visit(DataOperand& node) override { }
    void visit(IndexedAddress& node) override { }     // TODO: IMPLEMENT IF NEEDED
    void visit(PseudoMemory& node) override { }       // TODO: IMPLEMENT IF NEEDED
    void visit(CommentInstruction& node) override { } // NOT NEEDED
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override;
    void visit(MovsxInstruction& node) override;
    void visit(MovZeroExtendInstruction& node) override;
    void visit(LeaInstruction& node) override;
    void visit(Cvttsd2siInstruction& node) override;
    void visit(Cvtsi2sdInstruction& node) override;
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
    void visit(StaticConstant& node) override { } // TODO: IMPLEMENT IF NEEDED
    void visit(Program& node) override;

    size_t get_offset(AssemblyType type, const std::string& name);
    void check_and_replace(std::unique_ptr<Operand>& op);

    std::shared_ptr<AssemblyAST> m_ast;
    std::unordered_map<std::string, int> m_stack_offsets;
    std::shared_ptr<BackendSymbolTable> m_symbol_table;
    size_t m_curr_offset;

    // round-up to next multiple of alignment
    size_t round_up(size_t value, size_t alignment)
    {
        // Check if alignment is a power of 2
        if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
            // Handle non-power-of-2 alignments
            return ((value + alignment - 1) / alignment) * alignment;
        }

        // Optimized version for power-of-2 alignments
        return (value + alignment - 1) & ~(alignment - 1);
    }
};

} // namespace backend
