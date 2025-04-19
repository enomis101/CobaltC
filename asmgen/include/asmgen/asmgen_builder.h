#pragma once
#include "parser/parser_ast.h"
#include "asmgen/asmgen_ast.h"
#include <stdexcept>

namespace asmgen {


class AsmGenError : public std::runtime_error {
public:
    explicit AsmGenError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

//Generate an AsmGenAST from a ParserAST
class AssemblyGenerator : public parser::ParserVisitor {
public:
    AssemblyGenerator();

    std::unique_ptr<AsmGenAST> generate(parser::ParserAST* ast);
private:

    // Implementation of visitor interface methods
    void visit(parser::Identifier& node) override;
    void visit(parser::ConstantExpression& node) override;
    void visit(parser::ReturnStatement& node) override;
    void visit(parser::Function& node) override;
    void visit(parser::Program& node) override;
    
    std::unique_ptr<AsmGenAST> m_result;
    std::vector<std::unique_ptr<Instruction>> m_instructions_result;

    template<typename T>
    std::unique_ptr<T> consume_result() {
        if (!m_result) {
            throw AsmGenError("Result is null where a valid AST node was expected");
        }
        
        // Try to cast to the expected type
        T* result = dynamic_cast<T*>(m_result.get());
        if (!result) {
            throw AsmGenError("Expected AST node of type " + std::string(typeid(T).name()) + 
                              " but got " + std::string(typeid(m_result.get()).name()));
        }
        
        // Since the cast succeeded, we can safely move and static_cast
        return std::unique_ptr<T>(static_cast<T*>(m_result.release()));
    }

    std::vector<std::unique_ptr<Instruction>> consume_instructions_result();
};

} // namespace asmgen
