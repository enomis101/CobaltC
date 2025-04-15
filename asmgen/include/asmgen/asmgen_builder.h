#pragma once
#include "parser/parser_ast.h"
#include "asmgen/asmgen_ast.h"


namespace asmgen {

class AsmGenVisitor : public parser::ParserVisitor {
public:
    AsmGenVisitor();

    // Implementation of visitor interface methods
    void visit(parser::Identifier& node) override;
    void visit(parser::ConstantExpression& node) override;
    void visit(parser::ReturnStatement& node) override;
    void visit(parser::Function& node) override;
    void visit(parser::Program& node) override;

    std::unique_ptr<AsmGenAST> generate(parser::ParserAST* ast);
private:
    std::unique_ptr<AsmGenAST> m_result;

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
};

} // namespace asmgen
