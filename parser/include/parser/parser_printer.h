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
    void visit(ComplementOperator& node) { visit_operator(node); }
    void visit(NegateOperator& node) { visit_operator(node); }
    void visit(AddOperator& node) { visit_operator(node); }
    void visit(SubtractOperator& node) { visit_operator(node); }
    void visit(MultiplyOperator& node) { visit_operator(node); }
    void visit(DivideOperator& node) { visit_operator(node); }
    void visit(RemainderOperator& node) { visit_operator(node); }
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const ParserAST* node);

    template<typename NodeType>
    struct NodeTraits {
        static const char* name();
    };

    template<typename NodeType>
    void visit_operator(NodeType& node)
    {
        int id = get_node_id(&node);
        m_dot_content << "  node" << id << " [label=\"" << NodeTraits<NodeType>::name() << "\"];\n";
    }

    int m_node_count;                                     // Counter for generating unique node IDs
    std::unordered_map<const ParserAST*, int> m_node_ids; // Maps ParserAST nodes to their unique IDs
    std::stringstream m_dot_content;                      // Buffer for dot file content
};

// Specializations for each node type
template<>
const char* PrinterVisitor::NodeTraits<ComplementOperator>::name() { return "ComplementOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<NegateOperator>::name() { return "NegateOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<AddOperator>::name() { return "AddOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<SubtractOperator>::name() { return "SubtractOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<MultiplyOperator>::name() { return "MultiplyOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<DivideOperator>::name() { return "DivideOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<RemainderOperator>::name() { return "RemainderOperator"; }

} // namespace parser
