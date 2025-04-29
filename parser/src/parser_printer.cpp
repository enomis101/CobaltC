#include "parser/parser_printer.h"
#include <fstream>
#include <string>

using namespace parser;

PrinterVisitor::PrinterVisitor()
    : m_node_count(0)
{
}

void PrinterVisitor::generate_dot_file(const std::string& filename, ParserAST& ast)
{
    // Reset state for new file generation
    m_node_count = 0;
    m_node_ids.clear();
    m_dot_content.str("");

    // Start DOT file with digraph definition
    m_dot_content << "digraph ParserAST {\n";
    m_dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

    // Visit the ParserAST to build the DOT representation
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

void PrinterVisitor::visit(UnaryExpression& node)
{
    int id = get_node_id(&node);

    std::string label = "UnaryExpression";

    label += "\noperator: " + operator_to_string(node.unary_operator) + "\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(BinaryExpression& node)
{
    int id = get_node_id(&node);

    std::string label = "BinaryExpression";

    label += "\noperator: " + operator_to_string(node.binary_operator) + "\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.left_expression) {
        node.left_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.left_expression.get())
                      << " [label=\"left_expression\"];\n";
    }

    if (node.right_expression) {
        node.right_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.right_expression.get())
                      << " [label=\"right_expression\"];\n";
    }
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
    case BinaryOperator::AND:
        return "And";
    case BinaryOperator::OR:
        return "Or";
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

int PrinterVisitor::get_node_id(const ParserAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}
