#pragma once
#include "parser/parser_ast.h"
#include <sstream>
#include <unordered_map>

namespace parser {

class PrinterVisitor : public ParserVisitor {
public:
    PrinterVisitor();

    // Generate DOT file from the ParserAST
    void generate_dot_file(const std::string& filename, ParserAST& ast);

    // Implementation of visitor interface methods
    void visit(Identifier& node) override;
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(Program& node) override;
    void visit(VariableExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(FunctionCallExpression& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
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
    // Get or assign a unique ID for each node
    int get_node_id(const ParserAST* node);

    std::string operator_to_string(UnaryOperator op);
    std::string operator_to_string(BinaryOperator op);
    std::string storage_class_to_string(StorageClass sc);
    std::string declaration_scope_to_string(DeclarationScope scope);

    int m_node_count;                                     // Counter for generating unique node IDs
    std::unordered_map<const ParserAST*, int> m_node_ids; // Maps ParserAST nodes to their unique IDs
    std::stringstream m_dot_content;                      // Buffer for dot file content
};

} // namespace parser
