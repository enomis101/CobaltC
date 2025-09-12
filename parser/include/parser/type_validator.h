#pragma once
#include "parser/parser_ast.h"

namespace parser {

class TypeValidator : public ParserVisitor {
public:
    TypeValidator() = default;

    // Validate that all expressions and variables have valid types
    void validate_types(ParserAST& ast);

    // Implementation of visitor interface methods
    void visit(Identifier& node) override;
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(Program& node) override;
    void visit(VariableExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(FunctionCallExpression& node) override;
    void visit(DereferenceExpression& node) override;
    void visit(AddressOfExpression& node) override;
    void visit(SubscriptExpression& node) override;
    void visit(StringExpression& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(SingleInitializer& node) override;
    void visit(CompoundInitializer& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(Block& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ForInitDeclaration& node) override;
    void visit(ForInitExpression& node) override;

private:
    // Helper method to validate that a type is valid
    void validate_type(const std::unique_ptr<Type>& type, const std::string& node_name);
};

} // namespace parser
