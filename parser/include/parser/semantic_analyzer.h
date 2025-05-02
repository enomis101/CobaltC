#pragma once
#include "parser/parser_ast.h"

namespace parser{
    
class SemanticAnalyzer : ParserVisitor
{
private:
    void visit(Identifier& node) override;
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;
    void visit(VariableExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(VariableDeclaration& node) override;
};

}
