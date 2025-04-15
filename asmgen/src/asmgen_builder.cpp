#include "asmgen/asmgen_builder.h"
#include <fstream>
#include <string>

using namespace asmgen;

PrinterVisitor::PrinterVisitor()
    : m_node_count(0)
{
}

void PrinterVisitor::generate_dot_file(const std::string& filename, AST& ast)
{
    // Reset state for new file generation
    m_node_count = 0;
    m_node_ids.clear();
    m_dot_content.str("");

    // Start DOT file with digraph definition
    m_dot_content << "digraph AST {\n";
    m_dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

    // Visit the AST to build the DOT representation
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

void PrinterVisitor::visit(ConstantExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ConstantExpression\\nvalue: " << node.value << "\"];\n";
}

void PrinterVisitor::visit(ReturnStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ReturnStatement\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(Function& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Function\"];\n";

    if (node.name) {
        node.name->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.name.get())
                      << " [label=\"name\"];\n";
    }

    if (node.body) {
        node.body->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                      << " [label=\"body\"];\n";
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

int PrinterVisitor::get_node_id(const AST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}
