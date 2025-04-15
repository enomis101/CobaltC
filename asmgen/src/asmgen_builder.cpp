#include "asmgen/asmgen_builder.h"
#include <fstream>
#include <string>

using namespace asmgen;

AsmGenVisitor::AsmGenVisitor()
    : m_node_count(0)
{
}

void AsmGenVisitor::visit(parser::Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: " << node.name << "\"];\n";
}

void AsmGenVisitor::visit(parser::ConstantExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ConstantExpression\\nvalue: " << node.value << "\"];\n";
}

void AsmGenVisitor::visit(parser::ReturnStatement& node)
{
    if (!node.expression) {
        throw AsmGenError("Invalid parser::ReturnStatement node found during Assembly Generation");
    }
    node.expression->accept(*this);
}

void AsmGenVisitor::visit(parser::Function& node)
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

    if (!node.body) {
        throw AsmGenError("Invalid parser::Function node found during Assembly Generation");
    }
    node.body->accept(*this);

    std::unique_ptr<Instruction> instruction = consume_result<Instruction>();
    return std::make_unique<Function>(std::move(instruction));
}

void AsmGenVisitor::visit(parser::Program& node)
{
    if (!node.function) {
        throw AsmGenError("Invalid parser::Program node found during Assembly Generation");
    }
    node.function->accept(*this);
    std::unique_ptr<Function> function = consume_result<Function>();
    return std::make_unique<Program(std::move(function));
}

std::unique_ptr<AsmGenAST> AsmGenVisitor::generate(parser::ParserAST* ast) {
    ast->accept(*this);
    

    std::unique_ptr<Program> program = consume_result<Program>();
    return std::move(program);
}
