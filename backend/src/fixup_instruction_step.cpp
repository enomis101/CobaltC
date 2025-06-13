#include "backend/fixup_instruction_step.h"
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include <cstdint>
#include <variant>

using namespace backend;

FixUpInstructionsStep::FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<BackendSymbolTable> symbol_table)
    : m_ast { ast }
    , m_symbol_table { symbol_table }
{
    if (!m_ast || !dynamic_cast<Program*>(m_ast.get())) {
        throw FixUpInstructionsStepError("FixUpInstructionsStep: Invalid AST");
    }
}

void FixUpInstructionsStep::fixup()
{
    m_ast->accept(*this);
}

void FixUpInstructionsStep::visit(FunctionDefinition& node)
{
    std::vector<std::unique_ptr<Instruction>> tmp_instructions = std::move(node.instructions);
    node.instructions.clear();

    long stack_offset = round_up_to_16(std::get<FunctionEntry>(m_symbol_table->symbol_at(node.name.name)).stack_frame_size);
    node.instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::SUB, AssemblyType::QUAD_WORD, std::make_unique<ImmediateValue>(stack_offset), std::make_unique<Register>(RegisterName::SP)));
    for (auto& i : tmp_instructions) {
        if (MovInstruction* mov_instruction = dynamic_cast<MovInstruction*>(i.get())) {
            auto type = mov_instruction->type;

            // fixup_double_memory_address_instruction<MovInstruction>(i, node.instructions);
            if ((dynamic_cast<StackAddress*>(mov_instruction->source.get()) || dynamic_cast<DataOperand*>(mov_instruction->source.get()))
                && (dynamic_cast<StackAddress*>(mov_instruction->destination.get()) || dynamic_cast<DataOperand*>(mov_instruction->destination.get()))) {
                node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(mov_instruction->source), std::make_unique<Register>(RegisterName::R10)));
                mov_instruction->source = std::make_unique<Register>(RegisterName::R10);
            }
            // push a copy of instruction
            node.instructions.emplace_back(std::move(i));
        } else if (CmpInstruction* cmp_instruction = dynamic_cast<CmpInstruction*>(i.get())) {
            auto type = cmp_instruction->type;
            if (dynamic_cast<ImmediateValue*>(cmp_instruction->destination.get())) {
                // destination of cmp cant be a constant
                node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(cmp_instruction->destination), std::make_unique<Register>(RegisterName::R11)));
                cmp_instruction->destination = std::make_unique<Register>(RegisterName::R11);
            } else {
                // fixup_double_memory_address_instruction
                if ((dynamic_cast<StackAddress*>(cmp_instruction->source.get()) || dynamic_cast<DataOperand*>(cmp_instruction->source.get()))
                    && (dynamic_cast<StackAddress*>(cmp_instruction->destination.get()) || dynamic_cast<DataOperand*>(cmp_instruction->destination.get()))) {
                    node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(cmp_instruction->source), std::make_unique<Register>(RegisterName::R10)));
                    cmp_instruction->source = std::make_unique<Register>(RegisterName::R10);
                }
            }
            node.instructions.emplace_back(std::move(i));
        } else if (BinaryInstruction* binary_instruction = dynamic_cast<BinaryInstruction*>(i.get())) {
            auto type = binary_instruction->type;
            /*
            The quadword versions of our three binary arithmetic instructions (addq, imulq, and subq) can’t handle immediate values that don’t fit into an
            int, and neither can cmpq or pushq. If the source of any of these instructions is a constant outside the range of int, we’ll need to copy it into R10 before we can use it.
            */
            if (auto* imm_val = dynamic_cast<ImmediateValue*>(binary_instruction->source.get())) {
                if (std::holds_alternative<long>(imm_val->value) && (std::get<long>(imm_val->value) < INT32_MIN || std::get<long>(imm_val->value) > INT32_MAX)) {
                    node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(binary_instruction->source), std::make_unique<Register>(RegisterName::R10)));
                    binary_instruction->source = std::make_unique<Register>(RegisterName::R10);
                }
            }

            if ((binary_instruction->binary_operator == BinaryOperator::ADD) || (binary_instruction->binary_operator == BinaryOperator::SUB)) {
                // fixup_double_memory_address_instruction<BinaryInstruction>(i, node.instructions);
                if ((dynamic_cast<StackAddress*>(binary_instruction->source.get()) || dynamic_cast<DataOperand*>(binary_instruction->source.get()))
                    && (dynamic_cast<StackAddress*>(binary_instruction->destination.get()) || dynamic_cast<DataOperand*>(binary_instruction->destination.get()))) {
                    node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(binary_instruction->source), std::make_unique<Register>(RegisterName::R10)));
                    binary_instruction->source = std::make_unique<Register>(RegisterName::R10);
                }
                // push a copy of instruction
                node.instructions.emplace_back(std::move(i));
            } else if (binary_instruction->binary_operator == BinaryOperator::MULT) {
                // imul cant use memory addresses as its destination
                std::unique_ptr<Operand> destination_copy = binary_instruction->destination->clone();
                // Load destination into R11 register
                node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(binary_instruction->destination), std::make_unique<Register>(RegisterName::R11)));
                binary_instruction->destination = std::make_unique<Register>(RegisterName::R11);
                node.instructions.emplace_back(std::move(i));
                node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::make_unique<Register>(RegisterName::R11), std::move(destination_copy)));
            } else {
                node.instructions.push_back(std::move(i));
            }
        } else if (IdivInstruction* div_instruction = dynamic_cast<IdivInstruction*>(i.get())) {
            // idiv cant operate on immediate values:
            auto type = cmp_instruction->type;
            if (dynamic_cast<ImmediateValue*>(div_instruction->operand.get())) {
                node.instructions.emplace_back(std::make_unique<MovInstruction>(type, std::move(div_instruction->operand), std::make_unique<Register>(RegisterName::R10)));
                div_instruction->operand = std::make_unique<Register>(RegisterName::R10);
            }
            node.instructions.emplace_back(std::move(i));
        } else if (MovsxInstruction* movsx_instruction = dynamic_cast<MovsxInstruction*>(i.get())) {
            // movsx cant use a memory address as destination OR an immediate value as source.
            if (dynamic_cast<ImmediateValue*>(movsx_instruction->source.get())) {
                node.instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(movsx_instruction->source), std::make_unique<Register>(RegisterName::R10)));
                movsx_instruction->source = std::make_unique<Register>(RegisterName::R10);
            }
            if ((dynamic_cast<StackAddress*>(movsx_instruction->destination.get()) || dynamic_cast<DataOperand*>(movsx_instruction->destination.get()))) {
                node.instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::move(movsx_instruction->destination), std::make_unique<Register>(RegisterName::R11)));
                movsx_instruction->destination = std::make_unique<Register>(RegisterName::R11);
            }
            node.instructions.emplace_back(std::move(i));
        } else {
            node.instructions.emplace_back(std::move(i));
        }
    }
}

void FixUpInstructionsStep::visit(Program& node)
{
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}
