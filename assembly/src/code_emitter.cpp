#include "asmgen/code_emitter.h"
#include <filesystem>
#include <format>


using namespace asmgen;

CodeEmitter::CodeEmitter(const std::string& output_file, std::shared_ptr<AsmGenAST> ast)
    : m_output_file{output_file}, m_ast{ast}
{
    namespace fs = std::filesystem;

    bool file_exists = fs::exists(m_output_file);
    fs::path file_path = fs::path(m_output_file);
    if(!file_exists){
        //Check parent dir for write permission
        file_path = file_path.parent_path();
        if(file_path.empty()){
            file_path = ".";
        }
    }

    try {
        if(((fs::status(file_path).permissions() & fs::perms::owner_write) == fs::perms::none)) {
            throw CodeEmitterError(std::format("CodeEmitter: Invalid write permission for file {}", m_output_file));
        }
    } catch (const fs::filesystem_error& e) {
        throw CodeEmitterError(std::format("CodeEmitter: Failed to check permissions for {}: {}", 
                                           m_output_file, e.what()));
    }

    if(!m_ast || !dynamic_cast<Program*>(m_ast.get())){
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
    *m_file_stream << "%eax";
}

void CodeEmitter::visit(ReturnInstruction& node) 
{
    *m_file_stream << "ret";
}

void CodeEmitter::visit(MovInstruction& node) 
{
    *m_file_stream << "movl ";
    node.src->accept(*this);
    *m_file_stream << ", ";
    node.dst->accept(*this);
}

void CodeEmitter::visit(Function& node) 
{
    *m_file_stream << "\t.globl ";
    node.name->accept(*this);
    *m_file_stream << "\n";
    node.name->accept(*this);
    *m_file_stream << ":\n";
    for(auto& instruction : node.instructions){
        *m_file_stream << "\t";
        instruction->accept(*this);
        *m_file_stream << "\n";
    }
}

void CodeEmitter::visit(Program& node) 
{
    node.function->accept(*this);
    *m_file_stream << std::format("\t.section .note.GNU-stack,\"\",@progbits\n");
}