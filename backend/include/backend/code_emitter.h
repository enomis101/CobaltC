#pragma once
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace backend {

class CodeEmitterError : public std::runtime_error {
public:
    explicit CodeEmitterError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class CodeEmitter : public AssemblyVisitor {
public:
    CodeEmitter(const std::string& output_file, std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table);
    void emit_code();

private:
    void visit(Identifier& node) override { throw CodeEmitterError("visit(Identifier&) is not supported"); }
    void visit(ImmediateValue& node) override;
    void visit(Register& node) override;
    void visit(PseudoRegister& node) override { throw CodeEmitterError("Found PseudoRegister node during CodeEmission"); }
    void visit(StackAddress& node) override;
    void visit(DataOperand& node) override;
    void visit(CommentInstruction& node) override;
    void visit(ReturnInstruction& node) override;
    void visit(MovInstruction& node) override;
    void visit(MovsxInstruction& node) override;
    void visit(MovZeroExtendInstruction& node) override { throw CodeEmitterError("Found MovZeroExtendInstruction node during CodeEmission"); }
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(CmpInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(DivInstruction& node) override;
    void visit(CdqInstruction& node) override;
    void visit(JmpInstruction& node) override;
    void visit(JmpCCInstruction& node) override;
    void visit(SetCCInstruction& node) override;
    void visit(LabelInstruction& node) override;
    void visit(PushInstruction& node) override;
    void visit(CallInstruction& node) override;
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override;
    void visit(Program& node) override;

    std::string operator_instruction(UnaryOperator op);
    std::string operator_instruction(BinaryOperator op);
    std::string to_instruction_suffix(ConditionCode cc);
    std::string to_instruction_suffix(AssemblyType type);
    std::string get_function_name(const std::string& in_name);
    const std::string m_output_file;
    std::shared_ptr<AssemblyAST> m_ast;
    std::shared_ptr<BackendSymbolTable> m_symbol_table;
    std::ofstream* m_file_stream;
};

}
