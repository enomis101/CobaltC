#pragma once
#include "assembly/assembly_ast.h"
#include <sstream>
#include <unordered_map>

namespace assembly {

class PrinterVisitor : public AssemblyVisitor {
public:
    PrinterVisitor();

    // Generate DOT file from the AssemblyAST
    void generate_dot_file(const std::string& filename, AssemblyAST& ast);

    // Implementation of visitor interface methods
    void visit(Identifier& node) override;
    void visit(ImmediateValue& node) override;
    void visit(Register& node) override;
    void visit(PseudoRegister& node) override;
    void visit(StackAddress& node) override;
    void visit(NotOperator& node)  { visit_simple_node(node); }
    void visit(NegOperator& node)  { visit_simple_node(node); }
    void visit(AddOperator& node)  { visit_simple_node(node); }
    void visit(SubOperator& node)  { visit_simple_node(node); }
    void visit(MultOperator& node)  { visit_simple_node(node); }
    void visit(ReturnInstruction& node) { visit_simple_node(node); }
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(CdqInstruction& node) { visit_simple_node(node); }
    void visit(AllocateStackInstruction& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const AssemblyAST* node);

    template<typename NodeType>
    struct NodeTraits {
        static const char* name();
    };

    template<typename NodeType>
    void visit_simple_node(NodeType& node)
    {
        int id = get_node_id(&node);
        m_dot_content << "  node" << id << " [label=\"" << NodeTraits<NodeType>::name() << "\"];\n";
    }

    int m_node_count;                                       // Counter for generating unique node IDs
    std::unordered_map<const AssemblyAST*, int> m_node_ids; // Maps AssemblyAST nodes to their unique IDs
    std::stringstream m_dot_content;                        // Buffer for dot file content
};

// Specializations for each node type
template<>
const char* PrinterVisitor::NodeTraits<NotOperator>::name() { return "NotOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<NegOperator>::name() { return "NegOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<AddOperator>::name() { return "AddOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<SubOperator>::name() { return "SubOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<MultOperator>::name() { return "MultOperator"; }
template<>
const char* PrinterVisitor::NodeTraits<ReturnInstruction>::name() { return "ReturnInstruction"; }
template<>
const char* PrinterVisitor::NodeTraits<CdqInstruction>::name() { return "CdqInstruction"; }

} // namespace assembly
