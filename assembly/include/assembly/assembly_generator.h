#pragma once
#include "tacky/tacky_ast.h"
#include "assembly/assembly_ast.h"
#include <stdexcept>

namespace assembly {


class AssemblyGeneratorError : public std::runtime_error {
public:
    explicit AssemblyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

//Generate an AssemblyAST from a TackyAST
class AssemblyGenerator : public tacky::TackyVisitor {
public:
    AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast);

    std::unique_ptr<AssemblyAST> generate();
private:

    std::unique_ptr<Operand> transform_operand(tacky::Value& op);
    std::unique_ptr<UnaryOperator> transform_operator(tacky::UnaryOperator& op);
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::unique_ptr<Function> transform_function(tacky::Function& function);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    std::shared_ptr<tacky::TackyAST> m_ast;
};

} // namespace assembly
