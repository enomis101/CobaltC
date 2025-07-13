#include "backend/code_emitter.h"
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "common/error/internal_compiler_error.h"
#include <cassert>
#include <filesystem>
#include <format>
#include <variant>

using namespace backend;

CodeEmitter::CodeEmitter(const std::string& output_file, std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table)
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
    if (std::holds_alternative<int>(node.value)) {
        auto imm_value = std::get<int>(node.value);
        *m_file_stream << std::format("${}", imm_value);
    } else if (std::holds_alternative<long>(node.value)) {
        auto imm_value = std::get<long>(node.value);
        *m_file_stream << std::format("${}", imm_value);
    } else if (std::holds_alternative<unsigned int>(node.value)) {
        auto imm_value = std::get<unsigned int>(node.value);
        *m_file_stream << std::format("${}", imm_value);
    } else if (std::holds_alternative<unsigned long>(node.value)) {
        auto imm_value = std::get<unsigned long>(node.value);
        *m_file_stream << std::format("${}", imm_value);
    } else {
        assert(false);
    }
}

void CodeEmitter::visit(Register& node)
{
    switch (node.name) {
    case RegisterName::AX: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%rax";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%eax";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%al";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for AX");
        }
        break;
    }
    case RegisterName::DX: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%rdx";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%edx";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%dl";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for DX");
        }
        break;
    }
    case RegisterName::CX: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%rcx";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%ecx";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%cl";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for CX");
        }
        break;
    }
    case RegisterName::DI: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%rdi";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%edi";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%dil";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for DI");
        }
        break;
    }
    case RegisterName::SI: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%rsi";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%esi";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%sil";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for SI");
        }
        break;
    }
    case RegisterName::R8: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%r8";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%r8d";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%r8b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R8");
        }
        break;
    }
    case RegisterName::R9: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%r9";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%r9d";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%r9b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R9");
        }
        break;
    }
    case RegisterName::R10: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%r10";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%r10d";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%r10b";
            break;
        default:
            throw CodeEmitterError("CodeEmitter: Unsupported RegisterType for R10");
        }
        break;
    }
    case RegisterName::R11: {
        switch (node.type) {
        case AssemblyType::QUAD_WORD:
            *m_file_stream << "%r11";
            break;
        case AssemblyType::LONG_WORD:
            *m_file_stream << "%r11d";
            break;
        case AssemblyType::BYTE:
            *m_file_stream << "%r11b";
            break;
        default:
            throw InternalCompilerError("CodeEmitter: Unsupported RegisterType for R11");
        }
        break;
    }
    case RegisterName::SP: {
        *m_file_stream << "%rsp";
        break;
    }
    case RegisterName::BP: {
        if (node.type != AssemblyType::QUAD_WORD) {
            throw InternalCompilerError("Memory addresses are 8 byte");
        }
        *m_file_stream << "%rbp";
        break;
    }
    case RegisterName::XMM0:
        *m_file_stream << "%xmm0";
        break;
    case RegisterName::XMM1:
        *m_file_stream << "%xmm1";
        break;
    case RegisterName::XMM2:
        *m_file_stream << "%xmm2";
        break;
    case RegisterName::XMM3:
        *m_file_stream << "%xmm3";
        break;
    case RegisterName::XMM4:
        *m_file_stream << "%xmm4";
        break;
    case RegisterName::XMM5:
        *m_file_stream << "%xmm5";
        break;
    case RegisterName::XMM6:
        *m_file_stream << "%xmm6";
        break;
    case RegisterName::XMM7:
        *m_file_stream << "%xmm7";
        break;
    case RegisterName::XMM14:
        *m_file_stream << "%xmm14";
        break;
    case RegisterName::XMM15:
        *m_file_stream << "%xmm15";
        break;
    default:
        assert(false && "CodeEmitter: Unsupported RegisterName");
    }
}

void CodeEmitter::visit(MemoryAddress& node)
{
    *m_file_stream << std::format("{}(", node.offset);
    node.base_register->accept(*this);
    *m_file_stream << ")";
}

void CodeEmitter::visit(DataOperand& node)
{
    const std::string& data_name = node.identifier.name;
    std::string label_name = data_name;
    if (m_symbol_table->contains_symbol(data_name) && std::holds_alternative<ObjectEntry>(m_symbol_table->symbol_at(data_name))) {
        auto& obj_entry = std::get<ObjectEntry>(m_symbol_table->symbol_at(data_name));
        if (obj_entry.is_constant) {
            label_name = ".L" + data_name;
        }
    }

    *m_file_stream << std::format("{}(%rip)", label_name);
}

void CodeEmitter::visit(CommentInstruction& node)
{
    *m_file_stream << std::format("\t#{}\n", node.message);
}

void CodeEmitter::visit(ReturnInstruction& node)
{
    *m_file_stream << "\tmovq\t%rbp, %rsp\n";
    *m_file_stream << "\tpopq\t%rbp\n";
    *m_file_stream << "\tret\n";
}

void CodeEmitter::visit(MovInstruction& node)
{
    *m_file_stream << std::format("\tmov{} ", to_instruction_suffix(node.type));
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(MovsxInstruction& node)
{
    *m_file_stream << std::format("\tmovslq ");
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(LeaInstruction& node)
{
    *m_file_stream << std::format("\tleaq ");
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(Cvttsd2siInstruction& node)
{
    *m_file_stream << std::format("\tcvttsd2si{} ", to_instruction_suffix(node.type));
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(Cvtsi2sdInstruction& node)
{
    *m_file_stream << std::format("\tcvtsi2sd{} ", to_instruction_suffix(node.type));
    node.source->accept(*this);
    *m_file_stream << ", ";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(UnaryInstruction& node)
{
    *m_file_stream << std::format("\t{}{}\t", operator_instruction(node.unary_operator), to_instruction_suffix(node.type));
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(BinaryInstruction& node)
{
    if (node.binary_operator == BinaryOperator::XOR && node.type == AssemblyType::DOUBLE) {
        *m_file_stream << std::format("\txorpd\t");
    } else if (node.binary_operator == BinaryOperator::MULT && node.type == AssemblyType::DOUBLE) {
        *m_file_stream << std::format("\tmulsd\t");
    } else {
        *m_file_stream << std::format("\t{}{}\t", operator_instruction(node.binary_operator), to_instruction_suffix(node.type));
    }
    node.source->accept(*this);
    *m_file_stream << ",\t";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(CmpInstruction& node)
{
    if (node.type == AssemblyType::DOUBLE) {
        *m_file_stream << std::format("\tcomisd\t");
    } else {
        *m_file_stream << std::format("\tcmp{}\t", to_instruction_suffix(node.type));
    }
    node.source->accept(*this);
    *m_file_stream << ",\t";
    node.destination->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(IdivInstruction& node)
{
    *m_file_stream << std::format("\tidiv{}\t", to_instruction_suffix(node.type));
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(DivInstruction& node)
{
    *m_file_stream << std::format("\tdiv{}\t", to_instruction_suffix(node.type));
    node.operand->accept(*this);
    *m_file_stream << "\n";
}

void CodeEmitter::visit(CdqInstruction& node)
{
    if (node.type == AssemblyType::LONG_WORD) {
        *m_file_stream << "\tcdq\n";
    } else if (node.type == AssemblyType::QUAD_WORD) {
        *m_file_stream << "\tcqo\n";
    } else {
        assert(false);
    }
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

    /*TODO: FIX
    if (std::holds_alternative<int>(node.static_init)) {
        int init_value = std::get<int>(node.static_init);
        bool init_to_zero = init_value == 0;
        *m_file_stream << std::format("\t{}\n", (init_to_zero ? ".bss" : ".data"));
        *m_file_stream << std::format("\t.balign {}\n", node.alignment);
        *m_file_stream << std::format("{}:\n", node.name.name);
        *m_file_stream << std::format("\t{} {}\n", (init_to_zero ? ".zero" : ".long"), (init_to_zero ? 4 : init_value));
    } else if (std::holds_alternative<long>(node.static_init)) {
        long init_value = std::get<long>(node.static_init);
        bool init_to_zero = init_value == 0;
        *m_file_stream << std::format("\t{}\n", (init_to_zero ? ".bss" : ".data"));
        *m_file_stream << std::format("\t.balign {}\n", node.alignment);
        *m_file_stream << std::format("{}:\n", node.name.name);
        *m_file_stream << std::format("\t{} {}\n", (init_to_zero ? ".zero" : ".quad"), (init_to_zero ? 8 : init_value));
    } else if (std::holds_alternative<unsigned int>(node.static_init)) {
        auto init_value = std::get<unsigned int>(node.static_init);
        bool init_to_zero = init_value == 0;
        *m_file_stream << std::format("\t{}\n", (init_to_zero ? ".bss" : ".data"));
        *m_file_stream << std::format("\t.balign {}\n", node.alignment);
        *m_file_stream << std::format("{}:\n", node.name.name);
        *m_file_stream << std::format("\t{} {}\n", (init_to_zero ? ".zero" : ".long"), (init_to_zero ? 4 : init_value));
    } else if (std::holds_alternative<unsigned long>(node.static_init)) {
        auto init_value = std::get<unsigned long>(node.static_init);
        bool init_to_zero = init_value == 0;
        *m_file_stream << std::format("\t{}\n", (init_to_zero ? ".bss" : ".data"));
        *m_file_stream << std::format("\t.balign {}\n", node.alignment);
        *m_file_stream << std::format("{}:\n", node.name.name);
        *m_file_stream << std::format("\t{} {}\n", (init_to_zero ? ".zero" : ".quad"), (init_to_zero ? 8 : init_value));
    } else if (std::holds_alternative<double>(node.static_init)) {
        auto init_value = std::get<double>(node.static_init);
        *m_file_stream << std::format("\t{}\n", (".data"));
        *m_file_stream << std::format("\t.balign {}\n", node.alignment);
        *m_file_stream << std::format("{}:\n", node.name.name);
        *m_file_stream << std::format("\t{} {}\n", (".double"), (init_value));
    }
    */
}

void CodeEmitter::visit(StaticConstant& node)
{
    const auto& obj_attr = std::get<ObjectEntry>(m_symbol_table->symbol_at(node.name.name));

    *m_file_stream << std::format("\t.section .rodata\n");
    *m_file_stream << std::format("\t.balign {}\n", node.alignment);
    if (obj_attr.is_constant) {
        *m_file_stream << std::format(".L{}:\n", node.name.name);
    } else {
        *m_file_stream << std::format("{}:\n", node.name.name);
    }

    /*TODO: FIX
    if (std::holds_alternative<double>(node.static_init)) {
        auto init_value = std::get<double>(node.static_init);
        *m_file_stream << std::format("\t{} {}\n", (".double"), (init_value));
    } else {
        throw CodeEmitterError("CodeEmitter: Unsupported StaticConstant type");
    }
    */
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
        return "neg";
    case UnaryOperator::NOT:
        return "not";
    case UnaryOperator::SHR:
        return "shr";
    default:
        throw CodeEmitterError("CodeEmitter: Unsupported UnaryOperator");
    }
}

std::string CodeEmitter::operator_instruction(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "add";
    case BinaryOperator::SUB:
        return "sub";
    case BinaryOperator::MULT:
        return "imul";
    case BinaryOperator::DIV_DOUBLE:
        return "div";
    case BinaryOperator::AND:
        return "and";
    case BinaryOperator::OR:
        return "or";
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
    case ConditionCode::A:
        return "a";
    case ConditionCode::AE:
        return "ae";
    case ConditionCode::B:
        return "b";
    case ConditionCode::BE:
        return "be";
    default:
        assert(false);
    }
}

std::string CodeEmitter::to_instruction_suffix(AssemblyType type)
{
    switch (type) {
    case AssemblyType::LONG_WORD:
        return "l";
    case AssemblyType::QUAD_WORD:
        return "q";
    case AssemblyType::DOUBLE:
        return "sd";
    default:
        break;
    }
    assert(false && "CodeEmitter::to_instruction_suffix unsupported AssemblyType");
    return "NOT VALID";
}

std::string CodeEmitter::get_function_name(const std::string& in_name)
{
    const auto& fun_attr = std::get<FunctionEntry>(m_symbol_table->symbol_at(in_name));
    std::string suffix = fun_attr.defined ? "" : "@PLT";
    return in_name + suffix;
}
