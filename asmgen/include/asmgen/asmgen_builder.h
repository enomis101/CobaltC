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

};

} // namespace asmgen
