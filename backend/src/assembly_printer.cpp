#include "backend/assembly_printer.h"
#include <fstream>
#include <string>

using namespace backend;

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

std::string PrinterVisitor::register_name_to_string(RegisterName name)
{
    switch (name) {
    case RegisterName::AX:
        return "AX";
    case RegisterName::CX:
        return "CX";
    case RegisterName::DX:
        return "DX";
    case RegisterName::DI:
        return "DI";
    case RegisterName::SI:
        return "SI";
    case RegisterName::R8:
        return "R8";
    case RegisterName::R9:
        return "R9";
    case RegisterName::R10:
        return "R10";
    case RegisterName::R11:
        return "R11";
    case RegisterName::SP:
        return "SP";
    default:
        return "UNKNOWN";
    }
}

std::string PrinterVisitor::assembly_type_to_string(AssemblyType type)
{
    switch (type) {
    case AssemblyType::BYTE:
        return "BYTE (1-byte)";
    case AssemblyType::WORD:
        return "WORD (2-byte)";
    case AssemblyType::LONG_WORD:
        return "LONG_WORD (4-byte)";
    case AssemblyType::QUAD_WORD:
        return "QUAD_WORD (8-byte)";
    default:
        return "UNKNOWN";
    }
}

void PrinterVisitor::visit(Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: " << escape_string(node.name) << "\"];\n";
}

void PrinterVisitor::visit(ImmediateValue& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ImmediateValue\\nvalue: " << escape_string(constant_value_to_string(node.value)) << "\"];\n";
}

void PrinterVisitor::visit(Register& node)
{
    int id = get_node_id(&node);
    std::string reg_name = register_name_to_string(node.name);
    std::string type_name = assembly_type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"Register\\nname: " << reg_name
                  << "\\ntype: " << type_name << "\"];\n";
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

void PrinterVisitor::visit(DataOperand& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"DataOperand\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(ReturnInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ReturnInstruction\"];\n";
}

void PrinterVisitor::visit(MovInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"MovInstruction\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";

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

void PrinterVisitor::visit(MovsxInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"MovsxInstruction\"];\n";

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
    m_dot_content << "  node" << id << " [label=\"UnaryInstruction\\noperator: "
                  << operator_to_string(node.unary_operator) << "\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";

    if (node.operand) {
        node.operand->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.operand.get())
                      << " [label=\"operand\"];\n";
    }
}

void PrinterVisitor::visit(BinaryInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"BinaryInstruction\\noperator: "
                  << operator_to_string(node.binary_operator) << "\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";

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

void PrinterVisitor::visit(CmpInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CmpInstruction\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";

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
    m_dot_content << "  node" << id << " [label=\"IdivInstruction\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";

    if (node.operand) {
        node.operand->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.operand.get())
                      << " [label=\"operand\"];\n";
    }
}

void PrinterVisitor::visit(CdqInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CdqInstruction\\ntype: " << assembly_type_to_string(node.type) << "\"];\n";
}

void PrinterVisitor::visit(JmpInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"JmpInstruction\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(JmpCCInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"JmpCCInstruction\\ncondition: "
                  << operator_to_string(node.condition_code) << "\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(SetCCInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"SetCCInstruction\\ncondition: "
                  << operator_to_string(node.condition_code) << "\"];\n";

    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

void PrinterVisitor::visit(LabelInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"LabelInstruction\"];\n";

    // Visit the contained identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(PushInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"PushInstruction\"];\n";

    if (node.destination) {
        node.destination->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.destination.get())
                      << " [label=\"destination\"];\n";
    }
}

void PrinterVisitor::visit(CallInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CallInstruction\"];\n";

    // Visit the contained identifier
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

    // Process the instruction vector
    for (size_t i = 0; i < node.instructions.size(); ++i) {
        if (node.instructions[i]) {
            node.instructions[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.instructions[i].get())
                          << " [label=\"instructions[" << i << "]\"];\n";
        }
    }
}

void PrinterVisitor::visit(StaticVariable& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"StaticVariable\\nglobal: "
                  << (node.global ? "true" : "false")
                  << "\\nalignment: " << node.alignment
                  << "\\ninit: " << escape_string(constant_value_to_string(node.static_init)) << "\"];\n";

    // Visit the contained identifier
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

int PrinterVisitor::get_node_id(const AssemblyAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}

std::string PrinterVisitor::operator_to_string(UnaryOperator op)
{
    switch (op) {
    case UnaryOperator::NEG:
        return "NEG";
    case UnaryOperator::NOT:
        return "NOT";
    default:
        return "unknown";
    }
}

std::string PrinterVisitor::operator_to_string(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "ADD";
    case BinaryOperator::SUB:
        return "SUB";
    case BinaryOperator::MULT:
        return "MULT";
    default:
        return "unknown";
    }
}

std::string PrinterVisitor::operator_to_string(ConditionCode cc)
{
    switch (cc) {
    case ConditionCode::E:
        return "E";
    case ConditionCode::NE:
        return "NE";
    case ConditionCode::G:
        return "G";
    case ConditionCode::GE:
        return "GE";
    case ConditionCode::L:
        return "L";
    case ConditionCode::LE:
        return "LE";
    default:
        return "unknown";
    }
}
