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
    m_dot_content << "  node" << id << " [label=\"UnaryInstruction\"];\n";

    if (node.unary_operator) {
        node.unary_operator->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.unary_operator.get())
                      << " [label=\"unary_operator\"];\n";
    }

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
    m_dot_content << "  node" << id << " [label=\"BinaryInstruction\"];\n";

    if (node.binary_operator) {
        node.binary_operator->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.binary_operator.get())
                      << " [label=\"binary_operator\"];\n";
    }

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

void PrinterVisitor::visit(Function& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Function\"];\n";

    // Process the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    // Process the instruction vector
    for (size_t i = 0; i < node.body.size(); ++i) {
        if (node.body[i]) {
            node.body[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.body[i].get())
                          << " [label=\"body[" << i << "]\"];\n";
        }
    }
}

void PrinterVisitor::visit(Program& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Program\", color=blue, style=filled, fillcolor=lightblue];\n";

    if (node.function) {
        node.function->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.function.get())
                      << " [label=\"function\"];\n";
    }
}

int PrinterVisitor::get_node_id(const TackyAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}
