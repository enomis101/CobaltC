#include "tacky/tacky_printer.h"
#include <fstream>
#include <string>

using namespace tacky;

PrinterVisitor::PrinterVisitor()
    : m_node_count(0)
{
}

void PrinterVisitor::generate_dot_file(const std::string& filename, TackyAST& ast)
{
    // Reset state for new file generation
    m_node_count = 0;
    m_node_ids.clear();
    m_dot_content.str("");

    // Start DOT file with digraph definition
    m_dot_content << "digraph TackyAST {\n";
    m_dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

    // Visit the TackyAST to build the DOT representation
    ast.accept(*this);

    // Close the digraph
    m_dot_content << "}\n";

    // Write to file
    std::ofstream out_file(filename);
    if (out_file.is_open()) {
        out_file << m_dot_content.str();
        out_file.close();
    }
}

void PrinterVisitor::visit(Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: " << node.name << "\"];\n";
}

void PrinterVisitor::visit(Constant& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Constant\\nvalue: " << node.value << "\"];\n";
}

void PrinterVisitor::visit(TemporaryVariable& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"TemporaryVariable\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(ReturnInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ReturnInstruction\"];\n";

    if (node.value) {
        node.value->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.value.get())
                      << " [label=\"value\"];\n";
    }
}

void PrinterVisitor::visit(UnaryInstruction& node)
{
    int id = get_node_id(&node);

    std::string label = "UnaryInstruction";

    label += "\\noperator: " + operator_to_string(node.unary_operator) + "\\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.source) {
        node.source->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.source.get())
                      << " [label=\"source\"];\n";
    }

    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

void PrinterVisitor::visit(BinaryInstruction& node)
{
    int id = get_node_id(&node);

    std::string label = "BinaryInstruction";

    label += "\\noperator: " + operator_to_string(node.binary_operator) + "\\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.source1) {
        node.source1->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.source1.get())
                      << " [label=\"source1\"];\n";
    }

    if (node.source2) {
        node.source2->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.source2.get())
                      << " [label=\"source2\"];\n";
    }

    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

void PrinterVisitor::visit(CopyInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CopyInstruction\"];\n";

    if (node.source) {
        node.source->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.source.get())
                      << " [label=\"source\"];\n";
    }

    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

void PrinterVisitor::visit(JumpInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"JumpInstruction\"];\n";

    // Process the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(JumpIfZeroInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"JumpIfZeroInstruction\"];\n";

    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    // Process the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(JumpIfNotZeroInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"JumpIfNotZeroInstruction\"];\n";

    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    // Process the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(LabelInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"LabelInstruction\"];\n";

    // Process the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(FunctionDefinition& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"FunctionDefinition\"];\n";

    // Process the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    //TODO print glbl
    
    // Process the instruction vector
    for (size_t i = 0; i < node.body.size(); ++i) {
        if (node.body[i]) {
            node.body[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.body[i].get())
                          << " [label=\"body[" << i << "]\"];\n";
        }
    }
}

void PrinterVisitor::visit(StaticVariable& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"StaticVariable\"];\n";

    // Process the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";


    //TODO fix missing
}

void PrinterVisitor::visit(Program& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Program\", color=blue, style=filled, fillcolor=lightblue];\n";

    //TODO fix
    // Process each function in the vector
    // for (size_t i = 0; i < node.functions.size(); ++i) {
    //     if (node.functions[i]) {
    //         node.functions[i]->accept(*this);
    //         m_dot_content << "  node" << id << " -> node" << get_node_id(node.functions[i].get())
    //                       << " [label=\"functions[" << i << "]\"];\n";
    //     }
    // }
}

void PrinterVisitor::visit(FunctionCallInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"FunctionCallInstruction\"];\n";

    // Visit the function name
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    // Visit each argument
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        if (node.arguments[i]) {
            node.arguments[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.arguments[i].get())
                          << " [label=\"arguments[" << i << "]\"];\n";
        }
    }

    // Visit the destination
    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

int PrinterVisitor::get_node_id(const TackyAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}

std::string PrinterVisitor::operator_to_string(UnaryOperator op)
{
    switch (op) {
    case UnaryOperator::COMPLEMENT:
        return "Complement";
    case UnaryOperator::NEGATE:
        return "Negate";
    case UnaryOperator::NOT:
        return "Not";
    default:
        return "unknown";
    }
}

std::string PrinterVisitor::operator_to_string(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "Add";
    case BinaryOperator::SUBTRACT:
        return "Subtract";
    case BinaryOperator::MULTIPLY:
        return "Multiply";
    case BinaryOperator::DIVIDE:
        return "Divide";
    case BinaryOperator::REMAINDER:
        return "Remainder";
    case BinaryOperator::EQUAL:
        return "Equal";
    case BinaryOperator::NOT_EQUAL:
        return "NotEqual";
    case BinaryOperator::LESS_THAN:
        return "LessThan";
    case BinaryOperator::LESS_OR_EQUAL:
        return "LessOrEqual";
    case BinaryOperator::GREATER_THAN:
        return "GreaterThan";
    case BinaryOperator::GREATER_OR_EQUAL:
        return "GreaterOrEqual";
    default:
        return "unknown";
    }
}
