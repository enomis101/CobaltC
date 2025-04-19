#pragma once
#include "asmgen/asmgen_ast.h"
#include <memory>
#include <string>
#include <stdexcept>
#include <fstream>

namespace asmgen{

class CodeEmitterError : public std::runtime_error {
public:
    explicit CodeEmitterError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class CodeEmitter : public AsmGenVisitor{
public:
    CodeEmitter(const std::string& output_file, std::shared_ptr<AsmGenAST> ast);
    void emit_code();
private:
    void visit(Identifier& node) override;
    void visit(ImmediateValue& node) override;
    void visit(Register& node) override;
    void visit(ReturnInstruction& node) override;
    void visit(MovInstruction& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

    const std::string m_output_file;
    std::shared_ptr<AsmGenAST> m_ast;
    std::ofstream* m_file_stream;
};

}