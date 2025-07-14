#include "backend/fixup_instruction_step.h"
#include "backend/assembly_ast.h"
#include "backend/backend_symbol_table.h"
#include <cstdint>
#include <memory>
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
    // Move all existing instructions to temporary storage for processing
    std::vector<std::unique_ptr<Instruction>> tmp_instructions = std::move(node.instructions);
    node.instructions.clear();

    // Add stack frame allocation at the beginning using SUB instruction
    // Stack space must be aligned to 16 bytes as required by x86-64 ABI
    long stack_offset = round_up_to_16(std::get<FunctionEntry>(m_symbol_table->symbol_at(node.name.name)).stack_frame_size);
    node.instructions.emplace_back(std::make_unique<BinaryInstruction>(
        BinaryOperator::SUB,
        AssemblyType::QUAD_WORD,
        std::make_unique<ImmediateValue>(stack_offset),
        std::make_unique<Register>(RegisterName::SP)));

    // Process each instruction and apply necessary fixups
    std::vector<std::unique_ptr<Instruction>> tmp_instructions2;
    fixup_instructions(tmp_instructions, tmp_instructions2);

    // Do another pass to fix new mov instructions
    fixup_instructions(tmp_instructions2, node.instructions);
}

void FixUpInstructionsStep::fixup_instructions(std::vector<std::unique_ptr<Instruction>>& old_instructions, std::vector<std::unique_ptr<Instruction>>& new_instructions)
{
    for (auto& instruction : old_instructions) {
        if (dynamic_cast<MovInstruction*>(instruction.get())) {
            fixup_mov_instruction(instruction, new_instructions);
        } else if (dynamic_cast<CmpInstruction*>(instruction.get())) {
            fixup_cmp_instruction(instruction, new_instructions);
        } else if (dynamic_cast<BinaryInstruction*>(instruction.get())) {
            fixup_binary_instruction(instruction, new_instructions);
        } else if (dynamic_cast<IdivInstruction*>(instruction.get())) {
            fixup_idiv_instruction(instruction, new_instructions);
        } else if (dynamic_cast<DivInstruction*>(instruction.get())) {
            fixup_div_instruction(instruction, new_instructions);
        } else if (dynamic_cast<MovsxInstruction*>(instruction.get())) {
            fixup_movsx_instruction(instruction, new_instructions);
        } else if (dynamic_cast<MovZeroExtendInstruction*>(instruction.get())) {
            fixup_mov_zero_extend_instruction(instruction, new_instructions);
        } else if (dynamic_cast<PushInstruction*>(instruction.get())) {
            fixup_push_instruction(instruction, new_instructions);
        } else if (dynamic_cast<Cvttsd2siInstruction*>(instruction.get())) {
            fixup_cvttsd2si_instruction(instruction, new_instructions);
        } else if (dynamic_cast<Cvtsi2sdInstruction*>(instruction.get())) {
            fixup_cvtsi2sd_instruction(instruction, new_instructions);
        } else if (dynamic_cast<LeaInstruction*>(instruction.get())) {
            fixup_lea_instruction(instruction, new_instructions);
        } else {
            // No fixup needed for other instruction types
            new_instructions.emplace_back(std::move(instruction));
        }
    }
}

void FixUpInstructionsStep::fixup_mov_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto mov_instruction = dynamic_cast<MovInstruction*>(instruction.get());
    auto original_type = mov_instruction->type;

    if (mov_instruction->type == AssemblyType::DOUBLE) {
        // Only need to check memory-to-memory constraint
        bool source_is_memory = mov_instruction->source->is_memory();
        bool dest_is_memory = mov_instruction->destination->is_memory();

        if (source_is_memory && dest_is_memory) {
            // Use XMM register instead of R10
            instructions.emplace_back(std::make_unique<MovInstruction>(
                AssemblyType::DOUBLE,
                std::move(mov_instruction->source),
                std::make_unique<Register>(RegisterName::XMM14)));
            mov_instruction->source = std::make_unique<Register>(RegisterName::XMM14);
        }

        instructions.emplace_back(std::move(instruction));
        return; // Skip all the immediate handling code
    }

    // Handle large immediate values that exceed 32-bit signed integer range
    if (auto* imm_val = dynamic_cast<ImmediateValue*>(mov_instruction->source.get())) {
        if (std::holds_alternative<long>(imm_val->value)) {
            auto value = std::get<long>(imm_val->value);
            if (value < INT32_MIN || value > INT32_MAX) {
                if (original_type == AssemblyType::LONG_WORD) {
                    // For movl instructions, truncate 8-byte immediates to avoid assembler warnings
                    // The assembler would do this automatically, but we do it explicitly
                    imm_val->value = static_cast<int>(value);
                } else if (dynamic_cast<MemoryAddress*>(mov_instruction->destination.get()) || dynamic_cast<DataOperand*>(mov_instruction->destination.get())) {
                    // movq can move large immediates to registers but not directly to memory
                    // Use two-step process: immediate -> R10 -> memory
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        original_type,
                        std::move(mov_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    mov_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            }
        } else if (std::holds_alternative<unsigned long>(imm_val->value)) {
            auto value = std::get<unsigned long>(imm_val->value);
            if (value > static_cast<unsigned long>(INT32_MAX)) {
                if (original_type == AssemblyType::LONG_WORD) {
                    // For movl instructions, truncate 8-byte immediates to avoid assembler warnings
                    // The assembler would do this automatically, but we do it explicitly
                    imm_val->value = static_cast<int>(value);
                } else if (dynamic_cast<MemoryAddress*>(mov_instruction->destination.get()) || dynamic_cast<DataOperand*>(mov_instruction->destination.get())) {
                    // movq can move large immediates to registers but not directly to memory
                    // Use two-step process: immediate -> R10 -> memory
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        original_type,
                        std::move(mov_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    mov_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            }
        } else if (std::holds_alternative<unsigned int>(imm_val->value)) {
            auto value = std::get<unsigned int>(imm_val->value);
            if (value > static_cast<unsigned int>(INT32_MAX)) {
                if (original_type == AssemblyType::LONG_WORD) {
                    // For movl instructions, truncate to avoid assembler warnings
                    imm_val->value = static_cast<int>(value);
                } else if (dynamic_cast<MemoryAddress*>(mov_instruction->destination.get()) || dynamic_cast<DataOperand*>(mov_instruction->destination.get())) {
                    // movq can move large immediates to registers but not directly to memory
                    // Use two-step process: immediate -> R10 -> memory
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        original_type,
                        std::move(mov_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    mov_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            }
        }
    }

    // Handle memory-to-memory moves (not allowed in single x86-64 instruction)
    bool source_is_memory = mov_instruction->source->is_memory();
    bool dest_is_memory = mov_instruction->destination->is_memory();

    if (source_is_memory && dest_is_memory) {
        // Use two-step process: memory -> R10 -> memory
        instructions.emplace_back(std::make_unique<MovInstruction>(
            mov_instruction->type,
            std::move(mov_instruction->source),
            std::make_unique<Register>(RegisterName::R10)));
        mov_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
    }

    instructions.emplace_back(std::move(instruction));
}

void FixUpInstructionsStep::fixup_cmp_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto cmp_instruction = dynamic_cast<CmpInstruction*>(instruction.get());
    auto original_type = cmp_instruction->type;
    if (original_type == AssemblyType::DOUBLE) {
        if (!dynamic_cast<Register*>(cmp_instruction->destination.get())) {
            instructions.emplace_back(std::make_unique<MovInstruction>(
                AssemblyType::DOUBLE,
                std::move(cmp_instruction->destination),
                std::make_unique<Register>(RegisterName::XMM15)));
            cmp_instruction->destination = std::make_unique<Register>(RegisterName::XMM15);
        }
    } else {
        // Handle large immediate values in source operand
        // cmpq cannot handle immediates outside signed 32-bit range
        if (auto* imm_val = dynamic_cast<ImmediateValue*>(cmp_instruction->source.get())) {
            if (std::holds_alternative<long>(imm_val->value)) {
                long value = std::get<long>(imm_val->value);
                if (value < INT32_MIN || value > INT32_MAX) {
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        cmp_instruction->type,
                        std::move(cmp_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    cmp_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            } else if (std::holds_alternative<unsigned long>(imm_val->value)) {
                auto value = std::get<unsigned long>(imm_val->value);
                if (value > static_cast<unsigned long>(INT32_MAX)) {
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        original_type,
                        std::move(cmp_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    cmp_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            } else if (std::holds_alternative<unsigned int>(imm_val->value)) {
                auto value = std::get<unsigned int>(imm_val->value);
                if (value > static_cast<unsigned int>(INT32_MAX)) {
                    instructions.emplace_back(std::make_unique<MovInstruction>(
                        original_type,
                        std::move(cmp_instruction->source),
                        std::make_unique<Register>(RegisterName::R10)));
                    cmp_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
                }
            }
        }

        // Handle immediate value as destination (not allowed)
        if (dynamic_cast<ImmediateValue*>(cmp_instruction->destination.get())) {
            instructions.emplace_back(std::make_unique<MovInstruction>(
                cmp_instruction->type,
                std::move(cmp_instruction->destination),
                std::make_unique<Register>(RegisterName::R11)));
            cmp_instruction->destination = std::make_unique<Register>(RegisterName::R11, original_type);
        } else {
            // Handle memory-to-memory comparison (not allowed in single instruction)
            bool source_is_memory = cmp_instruction->source->is_memory();
            bool dest_is_memory = cmp_instruction->destination->is_memory();

            if (source_is_memory && dest_is_memory) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    cmp_instruction->type,
                    std::move(cmp_instruction->source),
                    std::make_unique<Register>(RegisterName::R10)));
                cmp_instruction->source = std::make_unique<Register>(RegisterName::R10, original_type);
            }
        }
    }
    instructions.emplace_back(std::move(instruction));
}

void FixUpInstructionsStep::fixup_binary_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto binary_instruction = dynamic_cast<BinaryInstruction*>(instruction.get());
    auto type = binary_instruction->type;

    // For ALL floating-point binary operations
    if (type == AssemblyType::DOUBLE) {
        // The destination of an addsd, subsd, mulsd, divsd, or xorpd instruction must be a register
        if (!dynamic_cast<Register*>(binary_instruction->destination.get())) {
            std::unique_ptr<Operand> destination_copy = binary_instruction->destination->clone();

            instructions.emplace_back(std::make_unique<MovInstruction>(
                type,
                std::move(binary_instruction->destination),
                std::make_unique<Register>(RegisterName::XMM14)));

            binary_instruction->destination = std::make_unique<Register>(RegisterName::XMM14);
            instructions.emplace_back(std::move(instruction));

            instructions.emplace_back(std::make_unique<MovInstruction>(
                type,
                std::make_unique<Register>(RegisterName::XMM14),
                std::move(destination_copy)));
        } else {
            instructions.emplace_back(std::move(instruction));
        }
        return; // Done - no need for other checks
    }

    // Handle large immediate values that exceed signed 32-bit range
    // addq, subq, and imulq cannot handle such large immediates
    if (auto* imm_val = dynamic_cast<ImmediateValue*>(binary_instruction->source.get())) {
        if (std::holds_alternative<long>(imm_val->value)) {
            auto value = std::get<long>(imm_val->value);
            if (value < INT32_MIN || value > INT32_MAX) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(binary_instruction->source),
                    std::make_unique<Register>(RegisterName::R10)));
                binary_instruction->source = std::make_unique<Register>(RegisterName::R10, type);
            }
        } else if (std::holds_alternative<unsigned long>(imm_val->value)) {
            auto value = std::get<unsigned long>(imm_val->value);
            if (value > static_cast<unsigned long>(INT32_MAX)) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(binary_instruction->source),
                    std::make_unique<Register>(RegisterName::R10)));
                binary_instruction->source = std::make_unique<Register>(RegisterName::R10, type);
            }
        } else if (std::holds_alternative<unsigned int>(imm_val->value)) {
            auto value = std::get<unsigned int>(imm_val->value);
            if (value > static_cast<unsigned int>(INT32_MAX)) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(binary_instruction->source),
                    std::make_unique<Register>(RegisterName::R10)));
                binary_instruction->source = std::make_unique<Register>(RegisterName::R10, type);
            }
        }
    }

    // Handle specific constraints for different binary operations
    if (binary_instruction->binary_operator == BinaryOperator::ADD || binary_instruction->binary_operator == BinaryOperator::SUB || binary_instruction->binary_operator == BinaryOperator::AND || binary_instruction->binary_operator == BinaryOperator::OR) {

        // ADD and SUB cannot have both operands as memory addresses
        bool source_is_memory = binary_instruction->source->is_memory();
        bool dest_is_memory = binary_instruction->destination->is_memory();

        if (source_is_memory && dest_is_memory) {
            instructions.emplace_back(std::make_unique<MovInstruction>(
                type,
                std::move(binary_instruction->source),
                std::make_unique<Register>(RegisterName::R10)));
            binary_instruction->source = std::make_unique<Register>(RegisterName::R10, type);
        }

        instructions.emplace_back(std::move(instruction));
    } else if (binary_instruction->binary_operator == BinaryOperator::MULT) {
        // IMUL cannot use memory addresses as destination operand
        // Use three-step process: load dest -> R11, perform imul, store R11 -> original dest
        std::unique_ptr<Operand> destination_copy = binary_instruction->destination->clone();

        instructions.emplace_back(std::make_unique<MovInstruction>(
            type,
            std::move(binary_instruction->destination),
            std::make_unique<Register>(RegisterName::R11)));

        binary_instruction->destination = std::make_unique<Register>(RegisterName::R11, type);
        instructions.emplace_back(std::move(instruction));

        instructions.emplace_back(std::make_unique<MovInstruction>(
            type,
            std::make_unique<Register>(RegisterName::R11),
            std::move(destination_copy)));
    } else {
        // No special fixup needed for other binary operations
        instructions.emplace_back(std::move(instruction));
    }
}

void FixUpInstructionsStep::fixup_idiv_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto div_instruction = dynamic_cast<IdivInstruction*>(instruction.get());
    // IDIV cannot operate directly on immediate values
    auto original_type = div_instruction->type;
    if (dynamic_cast<ImmediateValue*>(div_instruction->operand.get())) {
        instructions.emplace_back(std::make_unique<MovInstruction>(
            div_instruction->type,
            std::move(div_instruction->operand),
            std::make_unique<Register>(RegisterName::R10)));
        div_instruction->operand = std::make_unique<Register>(RegisterName::R10, original_type);
    }

    instructions.emplace_back(std::move(instruction));
}

void FixUpInstructionsStep::fixup_div_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto div_instruction = dynamic_cast<DivInstruction*>(instruction.get());
    // like IDIV, DIV cannot operate directly on immediate values
    auto original_type = div_instruction->type;
    if (dynamic_cast<ImmediateValue*>(div_instruction->operand.get())) {
        instructions.emplace_back(std::make_unique<MovInstruction>(
            div_instruction->type,
            std::move(div_instruction->operand),
            std::make_unique<Register>(RegisterName::R10)));
        div_instruction->operand = std::make_unique<Register>(RegisterName::R10, original_type);
    }

    instructions.emplace_back(std::move(instruction));
}

void FixUpInstructionsStep::fixup_push_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto push_instruction = dynamic_cast<PushInstruction*>(instruction.get());
    constexpr auto type = AssemblyType::QUAD_WORD;
    // pushq cannot handle immediate values outside signed 32-bit range
    if (auto imm_val = dynamic_cast<ImmediateValue*>(push_instruction->destination.get())) {
        if (std::holds_alternative<long>(imm_val->value)) {
            long value = std::get<long>(imm_val->value);
            if (value < INT32_MIN || value > INT32_MAX) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(push_instruction->destination),
                    std::make_unique<Register>(RegisterName::R10)));
                push_instruction->destination = std::make_unique<Register>(RegisterName::R10, type);
            }
        } else if (std::holds_alternative<unsigned long>(imm_val->value)) {
            auto value = std::get<unsigned long>(imm_val->value);
            if (value > static_cast<unsigned long>(INT32_MAX)) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(push_instruction->destination),
                    std::make_unique<Register>(RegisterName::R10)));
                push_instruction->destination = std::make_unique<Register>(RegisterName::R10, type);
            }
        } else if (std::holds_alternative<unsigned int>(imm_val->value)) {
            auto value = std::get<unsigned int>(imm_val->value);
            if (value > static_cast<unsigned int>(INT32_MAX)) {
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(push_instruction->destination),
                    std::make_unique<Register>(RegisterName::R10)));
                push_instruction->destination = std::make_unique<Register>(RegisterName::R10, type);
            }
        }
    }
    if (auto reg = dynamic_cast<Register*>(push_instruction->destination.get())) {
        if (is_xmm_register(reg->name)) {
            auto offset = std::make_unique<ImmediateValue>(8);
            auto dst = std::make_unique<Register>(RegisterName::SP);
            instructions.emplace_back(std::make_unique<BinaryInstruction>(BinaryOperator::SUB,
                AssemblyType::QUAD_WORD, std::move(offset), dst->clone()));
            // This should use MemoryAddress(SP, 0), not Register(SP)!
            instructions.emplace_back(std::make_unique<MovInstruction>(AssemblyType::DOUBLE,
                std::move(push_instruction->destination),
                std::make_unique<MemoryAddress>(RegisterName::SP, 0)));
            return;
        }
    }
    instructions.emplace_back(std::move(instruction));
}

void FixUpInstructionsStep::fixup_cvttsd2si_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto cvttsd2si_instruction = dynamic_cast<Cvttsd2siInstruction*>(instruction.get());
    std::unique_ptr<MovInstruction> mov_instruction = nullptr;
    if (!dynamic_cast<Register*>(cvttsd2si_instruction->destination.get())) {
        mov_instruction = std::make_unique<MovInstruction>(cvttsd2si_instruction->type, std::make_unique<Register>(RegisterName::R11), std::move(cvttsd2si_instruction->destination));
        cvttsd2si_instruction->destination = std::make_unique<Register>(RegisterName::R11, cvttsd2si_instruction->type);
    }
    instructions.emplace_back(std::move(instruction));
    if (mov_instruction) {
        instructions.emplace_back(std::move(mov_instruction));
    }
}

void FixUpInstructionsStep::fixup_cvtsi2sd_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto cvtsi2sd_instruction = dynamic_cast<Cvtsi2sdInstruction*>(instruction.get());
    std::unique_ptr<MovInstruction> mov_instruction1 = nullptr;
    std::unique_ptr<MovInstruction> mov_instruction2 = nullptr;
    if (dynamic_cast<ImmediateValue*>(cvtsi2sd_instruction->source.get())) {
        mov_instruction1 = std::make_unique<MovInstruction>(cvtsi2sd_instruction->type, std::move(cvtsi2sd_instruction->source), std::make_unique<Register>(RegisterName::R10));
        cvtsi2sd_instruction->source = std::make_unique<Register>(RegisterName::R10, cvtsi2sd_instruction->type);
    }
    if (!dynamic_cast<Register*>(cvtsi2sd_instruction->destination.get())) {
        mov_instruction2 = std::make_unique<MovInstruction>(AssemblyType::DOUBLE, std::make_unique<Register>(RegisterName::XMM15), std::move(cvtsi2sd_instruction->destination));
        cvtsi2sd_instruction->destination = std::make_unique<Register>(RegisterName::XMM15, AssemblyType::DOUBLE);
    }
    if (mov_instruction1) {
        instructions.emplace_back(std::move(mov_instruction1));
    }
    instructions.emplace_back(std::move(instruction));
    if (mov_instruction2) {
        instructions.emplace_back(std::move(mov_instruction2));
    }
}

void FixUpInstructionsStep::fixup_movsx_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto movsx_instruction = dynamic_cast<MovsxInstruction*>(instruction.get());
    // Handle immediate value as source (not allowed)
    if (dynamic_cast<ImmediateValue*>(movsx_instruction->source.get())) {
        // Move immediate to R10 with LONG_WORD size (source operand is 4 bytes)
        instructions.emplace_back(std::make_unique<MovInstruction>(
            AssemblyType::LONG_WORD,
            std::move(movsx_instruction->source),
            std::make_unique<Register>(RegisterName::R10)));
        movsx_instruction->source = std::make_unique<Register>(RegisterName::R10, AssemblyType::LONG_WORD);
    }

    // Handle memory address as destination (not allowed)
    std::unique_ptr<Instruction> additional_instruction = nullptr;
    bool dest_is_memory = movsx_instruction->destination->is_memory();

    if (dest_is_memory) {
        // Store original destination for final move (result is 8 bytes, so use QUAD_WORD)
        additional_instruction = std::make_unique<MovInstruction>(
            AssemblyType::QUAD_WORD,
            std::make_unique<Register>(RegisterName::R11),
            std::move(movsx_instruction->destination));
        movsx_instruction->destination = std::make_unique<Register>(RegisterName::R11, AssemblyType::QUAD_WORD);
    }

    // Add the fixed MOVSX instruction
    instructions.emplace_back(std::move(instruction));

    // Add the final move instruction if needed (must come after MOVSX)
    if (additional_instruction) {
        instructions.emplace_back(std::move(additional_instruction));
    }
}

void FixUpInstructionsStep::fixup_mov_zero_extend_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{

    auto mov_zero_extend_instruction = dynamic_cast<MovZeroExtendInstruction*>(instruction.get());
    bool dest_is_memory = mov_zero_extend_instruction->destination->is_memory();

    if (dest_is_memory) {
        auto mov_instr1 = std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(mov_zero_extend_instruction->source), std::make_unique<Register>(RegisterName::R11));
        instructions.emplace_back(std::move(mov_instr1));

        auto mov_instr2 = std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::make_unique<Register>(RegisterName::R11), std::move(mov_zero_extend_instruction->destination));
        instructions.emplace_back(std::move(mov_instr2));
    } else {
        auto mov_instr = std::make_unique<MovInstruction>(AssemblyType::LONG_WORD, std::move(mov_zero_extend_instruction->source), std::move(mov_zero_extend_instruction->destination));
        instructions.emplace_back(std::move(mov_instr));
    }
}

void FixUpInstructionsStep::fixup_lea_instruction(std::unique_ptr<Instruction>& instruction, std::vector<std::unique_ptr<Instruction>>& instructions)
{
    auto lea_instruction = dynamic_cast<LeaInstruction*>(instruction.get());
    bool dest_is_register = dynamic_cast<Register*>(lea_instruction->destination.get());
    if (!dest_is_register) {
        auto mov_instr = std::make_unique<MovInstruction>(AssemblyType::QUAD_WORD, std::make_unique<Register>(RegisterName::R11), std::move(lea_instruction->destination));
        lea_instruction->destination = std::make_unique<Register>(RegisterName::R11, AssemblyType::QUAD_WORD);
        instructions.emplace_back(std::move(instruction));
        instructions.emplace_back(std::move(mov_instr));
    } else {
        instructions.emplace_back(std::move(instruction));
    }
}

void FixUpInstructionsStep::visit(Program& node)
{
    // Process all function definitions in the program
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}

bool FixUpInstructionsStep::is_xmm_register(RegisterName reg)
{
    return reg >= RegisterName::XMM0 && reg <= RegisterName::XMM15;
}
