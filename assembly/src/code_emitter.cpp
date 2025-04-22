#include "assembly/code_emitter.h"
#include <filesystem>
#include <format>

using namespace assembly;

CodeEmitter::CodeEmitter(const std::string& output_file, std::shared_ptr<AssemblyAST> ast)
    : m_output_file { output_file }
    , m_ast { ast }
{
    namespace fs = std::filesystem;

    bool file_exists = fs::exists(m_output_file);
    fs::path file_path = fs::path(m_output_file);
    if (!file_exists) {
        // Check parent dir for write permission
        file_path = file_path.parent_path();
        if (file_path.empty()) {
            file_path = ".";
        }
    }

    try {
        if (((fs::status(file_path).permissions() & fs::perms::owner_write) == fs::perms::none)) {
            throw CodeEmitterError(std::format("CodeEmitter: Invalid write permission for file {}", m_output_file));
        }
    } catch (const fs::filesystem_error& e) {
        throw CodeEmitterError(std::format("CodeEmitter: Failed to check permissions for {}: {}",
            m_output_file, e.what()));
    }

    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw CodeEmitterError("CodeEmitter: Invalid AST");
    }
}

void CodeEmitter::emit_code()
{
    std::ofstream file_stream(m_output_file, std::ios::out | std::ios::trunc);
    m_file_stream = &file_stream;

    m_ast->accept(*this);
}

void CodeEmitter::visit(Identifier& node)
{
    *m_file_stream << std::format("{}", node.name);
}

void CodeEmitter::visit(ImmediateValue& node)
{
    *m_file_stream << std::format("${}", node.value);
}

void CodeEmitter::visit(Register& node)
{
    switch (node.reg) {
    case RegisterName::AX: {
        *m_file_stream << "%eax";
        break;
    }
    case RegisterName::R10: {
        *m_file_stream << "%r10d";
        break;
    }
    default:
        throw CodeEmitterError("CodeEmitter: Unsupported RegisterName");
        break;
    }
}

void CodeEmitter::visit(StackAddress& node)
{
    *m_file_stream << std::format("{}(%rbp)", node.offset);
}

void CodeEmitter::visit(NotOperator& node)
{
    *m_file_stream << "notl";
}

void CodeEmitter::visit(NegOperator& node)
{
    *m_file_stream << "negl";
}

void CodeEmitter::visit(ReturnInstruction& node)
{
    *m_file_stream << "\tmovq\t%rbp, %rsp\n";
    *m_file_stream << "\tpopq\t%rbp\n";
    *m_file_stream << "\tret\n";
}

void CodeEmitter::visit(MovInstruction& node)
{
    *m_file_stream << "\tmovl ";
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(UnaryInstruction& node)
{
    *m_file_stream << "\t";
    node.unary_operator->accept(*this);
    *m_file_stream << "\t";
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(AllocateStackInstruction& node)
{
    *m_file_stream << std::format("\tsubq\t${}, %rsp\n", node.value);
}

void CodeEmitter::visit(Function& node)
{
    *m_file_stream << "\t.globl ";
    node.name.accept(*this);
    *m_file_stream << "\n";
    node.name.accept(*this);
    *m_file_stream << ":\n";
    *m_file_stream << "\tpushq\t%rbp\n";
    *m_file_stream << "\tmovq\t%rsp, %rbp\n";
    for (auto& instruction : node.instructions) {
        instruction->accept(*this);
    }
}

void CodeEmitter::visit(Program& node)
{
    node.function->accept(*this);
    *m_file_stream << std::format("\t.section .note.GNU-stack,\"\",@progbits\n");
}
