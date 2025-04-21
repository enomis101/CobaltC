#pragma once
#include "tacky/tacky_ast.h"
#include <sstream>
#include <unordered_map>

namespace tacky {

class PrinterVisitor : public TackyVisitor {
public:
    PrinterVisitor();

    // Generate DOT file from the TackyAST
    void generate_dot_file(const std::string& filename, TackyAST& ast);

    // Implementation of visitor interface methods
    void visit(Identifier& node) override;
    void visit(ComplementOperator& node) override;
    void visit(NegateOperator& node) override;
    void visit(Constant& node) override;
    void visit(TemporaryVariable& node) override;
    void visit(ReturnInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const TackyAST* node);

    int m_node_count;                                    // Counter for generating unique node IDs
    std::unordered_map<const TackyAST*, int> m_node_ids; // Maps TackyAST nodes to their unique IDs
    std::stringstream m_dot_content;                     // Buffer for dot file content
};

} // namespace tacky
