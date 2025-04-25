#include "assembly/assembly_printer.h"
#include <fstream>
#include <string>

using namespace assembly;

PrinterVisitor::PrinterVisitor()
    : m_node_count(0)
{
}

void PrinterVisitor::generate_dot_file(const std::string& filename, AssemblyAST& ast)
{
    // Reset state for new file generation
    m_node_count = 0;
    m_node_ids.clear();
    m_dot_content.str("");

    // Start DOT file with digraph definition
    m_dot_content << "digraph AssemblyAST {\n";
    m_dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

    // Visit the AssemblyAST to build the DOT representation
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

void PrinterVisitor::visit(ImmediateValue& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ImmediateValue\\nvalue: " << node.value << "\"];\n";
}

void PrinterVisitor::visit(Register& node)
{
    int id = get_node_id(&node);
    std::string reg_name;
    switch (node.reg) {
    case RegisterName::AX:
        reg_name = "AX";
        break;
    case RegisterName::DX:
        reg_name = "DX";
        break;
    case RegisterName::R10:
        reg_name = "R10";
        break;
    case RegisterName::R11:
        reg_name = "R11";
        break;
    }
    m_dot_content << "  node" << id << " [label=\"Register\\nname: " << reg_name << "\"];\n";
}

void PrinterVisitor::visit(PseudoRegister& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"PseudoRegister\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(StackAddress& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"StackAddress\\noffset: " << node.offset << "\"];\n";
}

void PrinterVisitor::visit(MovInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"MovInstruction\"];\n";

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

void PrinterVisitor::visit(UnaryInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"UnaryInstruction\"];\n";

    if (node.unary_operator) {
        node.unary_operator->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.unary_operator.get())
                      << " [label=\"unary_operator\"];\n";
    }

    if (node.operand) {
        node.operand->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.operand.get())
                      << " [label=\"operand\"];\n";
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

void PrinterVisitor::visit(IdivInstruction& node) 
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"IdivInstruction\"];\n";

    if (node.operand) {
        node.operand->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.operand.get())
                      << " [label=\"operand\"];\n";
    }
}


void PrinterVisitor::visit(AllocateStackInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"AllocateStackInstruction\\nvalue: " << node.value << "\"];\n";
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
    for (size_t i = 0; i < node.instructions.size(); ++i) {
        if (node.instructions[i]) {
            node.instructions[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.instructions[i].get())
                          << " [label=\"instructions[" << i << "]\"];\n";
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

int PrinterVisitor::get_node_id(const AssemblyAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}
