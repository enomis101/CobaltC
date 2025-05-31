#include "assembly/code_emitter.h"
#include "common/data/symbol_table.h"
#include <filesystem>
#include <format>

using namespace assembly;

CodeEmitter::CodeEmitter(const std::string& output_file, std::shared_ptr<AssemblyAST> ast, std::shared_ptr<SymbolTable> symbol_table)
    : m_output_file { output_file }
    , m_ast { ast }
    , m_symbol_table { symbol_table }
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

void CodeEmitter::visit(ImmediateValue& node)
{
    *m_file_stream << std::format("${}", node.value);
}

void CodeEmitter::visit(Register& node)
{
    switch (node.reg) {
    case RegisterName::AX: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%rax";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%eax";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%al";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for AX");
        }
        break;
    }
    case RegisterName::DX: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%rdx";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%edx";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%dl";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for DX");
        }
        break;
    }
    case RegisterName::CX: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%rcx";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%ecx";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%cl";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for CX");
        }
        break;
    }
    case RegisterName::DI: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%rdi";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%edi";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%dil";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for DI");
        }
        break;
    }
    case RegisterName::SI: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%rsi";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%esi";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%sil";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for SI");
        }
        break;
    }
    case RegisterName::R8: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%r8";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%r8d";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%r8b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R8");
        }
        break;
    }
    case RegisterName::R9: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%r9";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%r9d";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%r9b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R9");
        }
        break;
    }
    case RegisterName::R10: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%r10";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%r10d";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%r10b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R10");
        }
        break;
    }
    case RegisterName::R11: {
        switch (node.type) {
        case RegisterType::QWORD:
            *m_file_stream << "%r11";
            break;
        case RegisterType::DWORD:
            *m_file_stream << "%r11d";
            break;
        case RegisterType::BYTE:
            *m_file_stream << "%r11b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R11");
        }
        break;
    }
    default:
        throw CodeEmitterError("CodeEmitter: Unsupported RegisterName");
    }
}

void CodeEmitter::visit(StackAddress& node)
{
    *m_file_stream << std::format("{}(%rbp)", node.offset);
}

void CodeEmitter::visit(DataOperand& node)
{
    *m_file_stream << std::format("{}(%rip)", node.identifier.name);
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
    *m_file_stream << std::format("\t{}\t", operator_instruction(node.unary_operator));
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(BinaryInstruction& node)
{
    *m_file_stream << std::format("\t{}\t", operator_instruction(node.binary_operator));
    node.source->accept(*this);
    *m_file_stream << ",\t";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(CmpInstruction& node)
{
    *m_file_stream << "\tcmpl\t";
    node.source->accept(*this);
    *m_file_stream << ",\t";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(IdivInstruction& node)
{
    *m_file_stream << "\tidivl\t";
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(CdqInstruction& node)
{
    *m_file_stream << "\tcdq\n";
}

void CodeEmitter::visit(JmpInstruction& node)
{
    *m_file_stream << std::format("\tjmp \t.L{}\n", node.identifier.name);
}

void CodeEmitter::visit(JmpCCInstruction& node)
{
    *m_file_stream << std::format("\tj{} \t.L{}\n", to_instruction_suffix(node.condition_code), node.identifier.name);
}

void CodeEmitter::visit(SetCCInstruction& node)
{
    *m_file_stream << std::format("\tset{} \t", to_instruction_suffix(node.condition_code));
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(LabelInstruction& node)
{
    *m_file_stream << std::format(".L{}:\n", node.identifier.name);
}

void CodeEmitter::visit(AllocateStackInstruction& node)
{
    *m_file_stream << std::format("\tsubq\t${}, %rsp\n", node.value);
}

void CodeEmitter::visit(DeallocateStackInstruction& node)
{
    *m_file_stream << std::format("\taddq\t${}, %rsp\n", node.value);
}
void CodeEmitter::visit(PushInstruction& node)
{
    *m_file_stream << "\tpushq\t";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}
void CodeEmitter::visit(CallInstruction& node)
{
    *m_file_stream << std::format("\tcall\t{}\n", get_function_name(node.identifier.name));
}

void CodeEmitter::visit(FunctionDefinition& node)
{
    if (node.global) {
        *m_file_stream << std::format("\t.globl {}\n", node.name.name);
    }
    *m_file_stream << "\t.text\n";
    *m_file_stream << std::format("{}:\n", node.name.name);
    *m_file_stream << "\tpushq\t%rbp\n";
    *m_file_stream << "\tmovq\t%rsp, %rbp\n";
    for (auto& instruction : node.instructions) {
        instruction->accept(*this);
    }
}

void CodeEmitter::visit(StaticVariable& node)
{
    if (node.global) {
        *m_file_stream << std::format("\t.globl {}\n", node.name.name);
    }
    bool init_to_zero = node.initializer == 0;
    *m_file_stream << std::format("\t{}\n", (init_to_zero ? ".bss" : ".data"));
    *m_file_stream << "\t.balign 4\n";
    *m_file_stream << std::format("{}:\n", node.name.name);
    *m_file_stream << std::format("\t{} {}\n", (init_to_zero ? ".zero" : ".long"), (init_to_zero ? 4 : node.initializer));
}

void CodeEmitter::visit(Program& node)
{
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
    *m_file_stream << std::format("\t.section .note.GNU-stack,\"\",@progbits\n");
}

std::string CodeEmitter::operator_instruction(UnaryOperator op)
{
    switch (op) {
    case UnaryOperator::NEG:
        return "negl";
    case UnaryOperator::NOT:
        return "notl";
    default:
        throw CodeEmitterError("CodeEmitter: Unsupported UnaryOperator");
    }
}

std::string CodeEmitter::operator_instruction(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "addl";
    case BinaryOperator::SUB:
        return "subl";
    case BinaryOperator::MULT:
        return "imull";
    default:
        throw CodeEmitterError("CodeEmitter: Unsupported BinaryOperator");
    }
}

std::string CodeEmitter::to_instruction_suffix(ConditionCode cc)
{
    switch (cc) {
    case ConditionCode::E:
        return "e";
    case ConditionCode::NE:
        return "ne";
    case ConditionCode::G:
        return "g";
    case ConditionCode::GE:
        return "ge";
    case ConditionCode::L:
        return "l";
    case ConditionCode::LE:
        return "le";
    default:
        return "unknown";
    }
}

std::string CodeEmitter::get_function_name(const std::string& in_name)
{
    FunctionAttribute& fun_attr = std::get<FunctionAttribute>(m_symbol_table->symbol_at(in_name).attribute);
    std::string suffix = fun_attr.defined ? "" : "@PLT";
    return in_name + suffix;
}
