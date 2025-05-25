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
    case RegisterName::CX:
        reg_name = "CX";
        break;
    case RegisterName::DI:
        reg_name = "DI";
        break;
    case RegisterName::SI:
        reg_name = "SI";
        break;
    case RegisterName::R8:
        reg_name = "R8";
        break;
    case RegisterName::R9:
        reg_name = "R9";
        break;
    case RegisterName::R10:
        reg_name = "R10";
        break;
    case RegisterName::R11:
        reg_name = "R11";
        break;
    default:
        reg_name = "UNKNOWN";
        break;
    }

    std::string type_name;
    switch (node.type) {
    case RegisterType::QWORD:
        type_name = "QWORD (8-byte)";
        break;
    case RegisterType::DWORD:
        type_name = "DWORD (4-byte)";
        break;
    case RegisterType::BYTE:
        type_name = "BYTE (1-byte)";
        break;
    default:
        type_name = "UNKNOWN";
        break;
    }

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

void PrinterVisitor::visit(ReturnInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ReturnInstruction\"];\n";
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
    m_dot_content << "  node" << id << " [label=\"UnaryInstruction\\noperator: "
                  << operator_to_string(node.unary_operator) << "\"];\n";

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
                  << operator_to_string(node.binary_operator) << "\"];\n";

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
    m_dot_content << "  node" << id << " [label=\"CmpInstruction\"];\n";

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

void PrinterVisitor::visit(CdqInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CdqInstruction\"];\n";
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

void PrinterVisitor::visit(AllocateStackInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"AllocateStackInstruction\\nvalue: " << node.value << "\"];\n";
}

void PrinterVisitor::visit(DeallocateStackInstruction& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"DeallocateStackInstruction\\nvalue: " << node.value << "\"];\n";
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
    m_dot_content << "  node" << id << " [label=\"FunctionDefinition\"];\n";

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

    // Process each function definition in the vector TODO: fix that
    // for (size_t i = 0; i < node.function_definitions.size(); ++i) {
    //     if (node.function_definitions[i]) {
    //         node.function_definitions[i]->accept(*this);
    //         m_dot_content << "  node" << id << " -> node" << get_node_id(node.function_definitions[i].get())
    //                       << " [label=\"function_definitions[" << i << "]\"];\n";
    //     }
    // }
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
