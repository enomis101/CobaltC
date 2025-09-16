#include "backend/pseudo_register_replace_step.h"
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include "common/error/internal_compiler_error.h"
#include <cassert>
#include <variant>

using namespace backend;

PseudoRegisterReplaceStep::PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table)
    : m_ast { ast }
    , m_symbol_table { symbol_table }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw PseudoRegisterReplaceStepError("PseudoRegisterReplaceStep: Invalid AST");
    }
}

void PseudoRegisterReplaceStep::replace()
{
    m_ast->accept(*this);
}

void PseudoRegisterReplaceStep::visit(MovInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(MovsxInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(MovZeroExtendInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(LeaInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(Cvttsd2siInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(Cvtsi2sdInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(UnaryInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(BinaryInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(CmpInstruction& node)
{
    check_and_replace(node.source);
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(SetCCInstruction& node)
{
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(IdivInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(DivInstruction& node)
{
    check_and_replace(node.operand);
}

void PseudoRegisterReplaceStep::visit(PushInstruction& node)
{
    check_and_replace(node.destination);
}

void PseudoRegisterReplaceStep::visit(FunctionDefinition& node)
{
    m_stack_offsets.clear();
    m_curr_offset = 0;

    for (auto& i : node.instructions) {
        i->accept(*this);
    }

    std::get<FunctionEntry>(m_symbol_table->symbol_at(node.name.name)).stack_frame_size = m_curr_offset;
}

void PseudoRegisterReplaceStep::visit(Program& node)
{
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}

void PseudoRegisterReplaceStep::check_and_replace(std::unique_ptr<Operand>& op)
{
    if (PseudoRegister* reg = dynamic_cast<PseudoRegister*>(op.get())) {
        const std::string& pseudo_reg_name = reg->identifier.name;
        std::unique_ptr<Operand> new_op = nullptr;
        /*
        When we encounter a pseudoregister that isn’t in m_stack_offsets, we look it up in the symbol table.
        If we find that it has static storage duration, we’ll map it to a Data operand by the same name. Otherwise, we’ll assign it a new slot on the stack, as usual.
        */
        bool is_static = m_symbol_table->contains_symbol(pseudo_reg_name) && std::get<ObjectEntry>(m_symbol_table->symbol_at(pseudo_reg_name)).is_static;
        if (!m_stack_offsets.contains(pseudo_reg_name) && is_static) {
            new_op = std::make_unique<DataOperand>(pseudo_reg_name);
        } else {
            if (!(m_symbol_table->contains_symbol(pseudo_reg_name) && std::holds_alternative<ObjectEntry>(m_symbol_table->symbol_at(pseudo_reg_name)))) {
                throw InternalCompilerError("PseudoRegister not contained in the symbol table");
            }

            AssemblyType type = std::get<ObjectEntry>(m_symbol_table->symbol_at(pseudo_reg_name)).type;
            size_t offset = get_offset(type, pseudo_reg_name);
            new_op = std::make_unique<MemoryAddress>(RegisterName::BP, -offset);
        }
        op = std::move(new_op);
    } else if (auto mem = dynamic_cast<PseudoMemory*>(op.get())) {
        const std::string& pseudo_mem_name = mem->identifier.name;
        std::unique_ptr<Operand> new_op = nullptr;
        /*
        When we encounter a pseudoregister that isn’t in m_stack_offsets, we look it up in the symbol table.
        If we find that it has static storage duration, we’ll map it to a Data operand by the same name. Otherwise, we’ll assign it a new slot on the stack, as usual.
        */
        bool is_static = m_symbol_table->contains_symbol(pseudo_mem_name) && std::get<ObjectEntry>(m_symbol_table->symbol_at(pseudo_mem_name)).is_static;
        if (!m_stack_offsets.contains(pseudo_mem_name) && is_static) {
            new_op = std::make_unique<DataOperand>(pseudo_mem_name);
        } else {
            if (!(m_symbol_table->contains_symbol(pseudo_mem_name) && std::holds_alternative<ObjectEntry>(m_symbol_table->symbol_at(pseudo_mem_name)))) {
                throw InternalCompilerError("PseudoMemory not contained in the symbol table");
            }

            AssemblyType type = std::get<ObjectEntry>(m_symbol_table->symbol_at(pseudo_mem_name)).type;
            size_t offset = get_offset(type, pseudo_mem_name);
            offset -= mem->offset;
            new_op = std::make_unique<MemoryAddress>(RegisterName::BP, -offset);
        }
        op = std::move(new_op);
    }
}

size_t PseudoRegisterReplaceStep::get_offset(AssemblyType type, const std::string& name)
{
    if (!m_stack_offsets.contains(name)) {
        if (type == AssemblyType::BYTE) {
            m_curr_offset++;
        } else if (type == AssemblyType::LONG_WORD) {
            m_curr_offset += 4;
        } else if (type == AssemblyType::QUAD_WORD || type == AssemblyType::DOUBLE) {
            m_curr_offset = round_up(m_curr_offset + 8, 8);
        } else if (type == AssemblyType::BYTE_ARRAY) {
            m_curr_offset = round_up(m_curr_offset + type.size(), type.alignment());
        }
        m_stack_offsets[name] = m_curr_offset;
    }

    return m_stack_offsets[name];
}
