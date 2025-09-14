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
    void visit(Constant& node) override;
    void visit(TemporaryVariable& node) override;
    void visit(ReturnInstruction& node) override;
    void visit(SignExtendInstruction& node) override;
    void visit(TruncateInstruction& node) override;
    void visit(ZeroExtendInstruction& node) override;
    void visit(DoubleToIntIntruction& node) override;
    void visit(DoubleToUIntIntruction& node) override;
    void visit(IntToDoubleIntruction& node) override;
    void visit(UIntToDoubleIntruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(CopyInstruction& node) override;
    void visit(GetAddressInstruction& node) override;
    void visit(LoadInstruction& node) override;
    void visit(StoreInstruction& node) override;
    void visit(AddPointerInstruction& node) override;
    void visit(CopyToOffsetInstruction& node) override;
    void visit(JumpInstruction& node) override;
    void visit(JumpIfZeroInstruction& node) override;
    void visit(JumpIfNotZeroInstruction& node) override;
    void visit(LabelInstruction& node) override;
    void visit(FunctionCallInstruction& node) override;
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override;
    void visit(StaticConstant& node) override;
    void visit(Program& node) override;

private:
    // Get or assign a unique ID for each node
    int get_node_id(const TackyAST* node);

    std::string operator_to_string(UnaryOperator op);
    std::string operator_to_string(BinaryOperator op);
    std::string constant_value_to_string(const ConstantType& value);
    std::string escape_string(const std::string& str);

    int m_node_count;                                    // Counter for generating unique node IDs
    std::unordered_map<const TackyAST*, int> m_node_ids; // Maps TackyAST nodes to their unique IDs
    std::stringstream m_dot_content;                     // Buffer for dot file content
};

} // namespace tacky
