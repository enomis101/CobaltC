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

std::string PrinterVisitor::constant_value_to_string(const ConstantType& value)
{
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "[uninitialized]";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, long>) {
            return std::to_string(v) + "L";
        } else {
            return "[unknown_type]";
        }
    },
        value);
}

std::string PrinterVisitor::escape_string(const std::string& str)
{
    std::string result;
    for (char c : str) {
        switch (c) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
            break;
        }
    }
    return result;
}

void PrinterVisitor::visit(Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: " << escape_string(node.name) << "\"];\n";
}

void PrinterVisitor::visit(Constant& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Constant\\nvalue: " << escape_string(constant_value_to_string(node.value)) << "\"];\n";
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

void PrinterVisitor::visit(SignExtendInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"SignExtendInstruction\"];\n";

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

void PrinterVisitor::visit(TruncateInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"TruncateInstruction\"];\n";

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

void PrinterVisitor::visit(ZeroExtendInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ZeroExtendInstruction\"];\n";

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

void PrinterVisitor::visit(DoubleToIntIntruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"DoubleToIntIntruction\"];\n";

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

void PrinterVisitor::visit(DoubleToUIntIntruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"DoubleToUIntIntruction\"];\n";

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

void PrinterVisitor::visit(IntToDoubleIntruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"IntToDoubleIntruction\"];\n";

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

void PrinterVisitor::visit(UIntToDoubleIntruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"UIntToDoubleIntruction\"];\n";

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
    m_dot_content << "  node" << id << " [label=\"FunctionDefinition\\nglobal: "
                  << (node.global ? "true" : "false") << "\"];\n";

    // Process the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    // Process the parameters
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        node.parameters[i].accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.parameters[i])
                      << " [label=\"parameters[" << i << "]\"];\n";
    }

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
    m_dot_content << "  node" << id << " [label=\"StaticVariable\\nglobal: "
                  << (node.global ? "true" : "false")
                  << "\\ninit: " << escape_string(constant_value_to_string(node.init)) << "\"];\n";

    // Process the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";
}

void PrinterVisitor::visit(Program& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Program\", color=blue, style=filled, fillcolor=lightblue];\n";

    // Process each definition in the vector
    for (size_t i = 0; i < node.definitions.size(); ++i) {
        if (node.definitions[i]) {
            node.definitions[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.definitions[i].get())
                          << " [label=\"definitions[" << i << "]\"];\n";
        }
    }
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
