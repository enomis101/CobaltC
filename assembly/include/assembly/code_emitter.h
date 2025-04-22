#pragma once
#include "assembly/assembly_ast.h"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>

namespace assembly {

class CodeEmitterError : public std::runtime_error {
public:
    explicit CodeEmitterError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class CodeEmitter : public AssemblyVisitor {
public:
    CodeEmitter(const std::string& output_file, std::shared_ptr<AssemblyAST> ast);
    void emit_code();

private:
    void visit(Identifier& node) override;
    void visit(ImmediateValue& node) override;
    void visit(Register& node) override;
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override;
    void visit(NotOperator& node) override;
    void visit(NegOperator& node) override;
    void visit(ReturnInstruction& node) override;
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(AllocateStackInstruction& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

    const std::string m_output_file;
    std::shared_ptr<AssemblyAST> m_ast;
    std::ofstream* m_file_stream;
};

}
