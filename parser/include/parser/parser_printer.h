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
    void visit(Function& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const ParserAST* node);

    std::string operator_to_string(UnaryOperator op);
    std::string operator_to_string(BinaryOperator op);

    int m_node_count;                                     // Counter for generating unique node IDs
    std::unordered_map<const ParserAST*, int> m_node_ids; // Maps ParserAST nodes to their unique IDs
    std::stringstream m_dot_content;                      // Buffer for dot file content
};

} // namespace parser
