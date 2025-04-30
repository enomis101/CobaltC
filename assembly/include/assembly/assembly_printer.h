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
    void visit(ReturnInstruction& node) override;
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(CmpInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(CdqInstruction& node) override;
    void visit(JumpInstruction& node) override;
    void visit(JumpCCInstruction& node) override;
    void visit(SetCCInstruction& node) override;
    void visit(LabelInstruction& node) override;
    void visit(AllocateStackInstruction& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const AssemblyAST* node);

    std::string operator_to_string(UnaryOperator op);
    std::string operator_to_string(BinaryOperator op);
    std::string operator_to_string(ConditionCode cc);

    int m_node_count;                                       // Counter for generating unique node IDs
    std::unordered_map<const AssemblyAST*, int> m_node_ids; // Maps AssemblyAST nodes to their unique IDs
    std::stringstream m_dot_content;                        // Buffer for dot file content
};

} // namespace assembly
