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

    // Add stack frame allocation at the beginning of the function
    // This must be aligned to 16 bytes as required by x86-64 ABI
    long stack_offset = round_up_to_16(std::get<FunctionEntry>(m_symbol_table->symbol_at(node.name.name)).stack_frame_size);
    node.instructions.emplace_back(std::make_unique<BinaryInstruction>(
        BinaryOperator::SUB,
        AssemblyType::QUAD_WORD,
        std::make_unique<ImmediateValue>(stack_offset),
        std::make_unique<Register>(RegisterName::SP)));

    // Process each instruction and apply necessary fixups
    for (auto& instruction : tmp_instructions) {
        if (MovInstruction* mov_instruction = dynamic_cast<MovInstruction*>(instruction.get())) {
            fixup_mov_instruction(mov_instruction, node.instructions);
            node.instructions.emplace_back(std::move(instruction));
        } else if (CmpInstruction* cmp_instruction = dynamic_cast<CmpInstruction*>(instruction.get())) {
            fixup_cmp_instruction(cmp_instruction, node.instructions);
            node.instructions.emplace_back(std::move(instruction));
        } else if (BinaryInstruction* binary_instruction = dynamic_cast<BinaryInstruction*>(instruction.get())) {
            fixup_binary_instruction(binary_instruction, node.instructions, instruction);
        } else if (IdivInstruction* div_instruction = dynamic_cast<IdivInstruction*>(instruction.get())) {
            fixup_idiv_instruction(div_instruction, node.instructions);
            node.instructions.emplace_back(std::move(instruction));
        } else if (MovsxInstruction* movsx_instruction = dynamic_cast<MovsxInstruction*>(instruction.get())) {
            fixup_movsx_instruction(movsx_instruction, node.instructions, instruction);
        } else {
            // No fixup needed for other instruction types
            node.instructions.emplace_back(std::move(instruction));
        }
    }
}

void FixUpInstructionsStep::fixup_mov_instruction(MovInstruction* mov_instruction,
    std::vector<std::unique_ptr<Instruction>>& instructions)
{
    /*
     * x86-64 MOV instruction constraint fixup:
     * The x86-64 architecture does not allow memory-to-memory moves in a single instruction.
     * If both source and destination are memory operands (either stack addresses or data operands),
     * we must use an intermediate register to perform the operation in two steps:
     * 1. Move from source memory to intermediate register (R10)
     * 2. Move from intermediate register to destination memory
     */

    bool source_is_memory = dynamic_cast<StackAddress*>(mov_instruction->source.get()) || dynamic_cast<DataOperand*>(mov_instruction->source.get());
    bool dest_is_memory = dynamic_cast<StackAddress*>(mov_instruction->destination.get()) || dynamic_cast<DataOperand*>(mov_instruction->destination.get());

    if (source_is_memory && dest_is_memory) {
        // Insert intermediate move: source -> R10
        instructions.emplace_back(std::make_unique<MovInstruction>(
            mov_instruction->type,
            std::move(mov_instruction->source),
            std::make_unique<Register>(RegisterName::R10)));
        // Update the original instruction to use R10 as source
        mov_instruction->source = std::make_unique<Register>(RegisterName::R10);
    }
}

void FixUpInstructionsStep::fixup_cmp_instruction(CmpInstruction* cmp_instruction,
    std::vector<std::unique_ptr<Instruction>>& instructions)
{
    /*
     * x86-64 CMP instruction constraint fixups:
     * 1. CMP cannot have an immediate value as its destination operand
     * 2. CMP cannot have both operands as memory addresses (same as MOV)
     */

    // Fix immediate value as destination
    if (dynamic_cast<ImmediateValue*>(cmp_instruction->destination.get())) {
        // Move the immediate value to R11 register first
        instructions.emplace_back(std::make_unique<MovInstruction>(
            cmp_instruction->type,
            std::move(cmp_instruction->destination),
            std::make_unique<Register>(RegisterName::R11)));
        cmp_instruction->destination = std::make_unique<Register>(RegisterName::R11);
    } else {
        // Fix double memory address case (similar to MOV instruction)
        bool source_is_memory = dynamic_cast<StackAddress*>(cmp_instruction->source.get()) || dynamic_cast<DataOperand*>(cmp_instruction->source.get());
        bool dest_is_memory = dynamic_cast<StackAddress*>(cmp_instruction->destination.get()) || dynamic_cast<DataOperand*>(cmp_instruction->destination.get());

        if (source_is_memory && dest_is_memory) {
            // Move source to intermediate register R10
            instructions.emplace_back(std::make_unique<MovInstruction>(
                cmp_instruction->type,
                std::move(cmp_instruction->source),
                std::make_unique<Register>(RegisterName::R10)));
            cmp_instruction->source = std::make_unique<Register>(RegisterName::R10);
        }
    }
}

void FixUpInstructionsStep::fixup_binary_instruction(BinaryInstruction* binary_instruction,
    std::vector<std::unique_ptr<Instruction>>& instructions,
    std::unique_ptr<Instruction>& instruction)
{
    /*
     * x86-64 Binary instruction constraint fixups:
     * 1. Quadword arithmetic operations (addq, subq, imulq) cannot handle immediate values
     *    that don't fit in a 32-bit signed integer (must be in range INT32_MIN to INT32_MAX)
     * 2. ADD and SUB cannot have both operands as memory addresses
     * 3. IMUL (multiplication) cannot use memory addresses as destination operand
     */

    auto type = binary_instruction->type;

    // Fix large immediate values that don't fit in int32
    if (auto* imm_val = dynamic_cast<ImmediateValue*>(binary_instruction->source.get())) {
        if (std::holds_alternative<long>(imm_val->value)) {
            long value = std::get<long>(imm_val->value);
            if (value < INT32_MIN || value > INT32_MAX) {
                // Move large immediate to R10 register first
                instructions.emplace_back(std::make_unique<MovInstruction>(
                    type,
                    std::move(binary_instruction->source),
                    std::make_unique<Register>(RegisterName::R10)));
                binary_instruction->source = std::make_unique<Register>(RegisterName::R10);
            }
        }
    }

    // Handle specific binary operations
    if (binary_instruction->binary_operator == BinaryOperator::ADD || binary_instruction->binary_operator == BinaryOperator::SUB) {

        // Fix double memory address case for ADD/SUB
        bool source_is_memory = dynamic_cast<StackAddress*>(binary_instruction->source.get()) || dynamic_cast<DataOperand*>(binary_instruction->source.get());
        bool dest_is_memory = dynamic_cast<StackAddress*>(binary_instruction->destination.get()) || dynamic_cast<DataOperand*>(binary_instruction->destination.get());

        if (source_is_memory && dest_is_memory) {
            // Move source to intermediate register R10
            instructions.emplace_back(std::make_unique<MovInstruction>(
                type,
                std::move(binary_instruction->source),
                std::make_unique<Register>(RegisterName::R10)));
            binary_instruction->source = std::make_unique<Register>(RegisterName::R10);
        }

        instructions.emplace_back(std::move(instruction));
    } else if (binary_instruction->binary_operator == BinaryOperator::MULT) {
        // IMUL special case: cannot use memory as destination
        // We need to: load dest -> R11, perform imul R11, store R11 -> original_dest
        std::unique_ptr<Operand> destination_copy = binary_instruction->destination->clone();

        // Load destination value into R11
        instructions.emplace_back(std::make_unique<MovInstruction>(
            type,
            std::move(binary_instruction->destination),
            std::make_unique<Register>(RegisterName::R11)));

        // Update instruction to use R11 as destination
        binary_instruction->destination = std::make_unique<Register>(RegisterName::R11);

        // Add the multiplication instruction
        instructions.emplace_back(std::move(instruction));

        // Store result back to original destination
        instructions.emplace_back(std::make_unique<MovInstruction>(
            type,
            std::make_unique<Register>(RegisterName::R11),
            std::move(destination_copy)));
    } else {
        // No special fixup needed for other binary operations
        instructions.push_back(std::move(instruction));
    }
}

void FixUpInstructionsStep::fixup_idiv_instruction(IdivInstruction* div_instruction,
    std::vector<std::unique_ptr<Instruction>>& instructions)
{
    /*
     * x86-64 IDIV instruction constraint fixup:
     * The IDIV instruction cannot operate directly on immediate values.
     * If the operand is an immediate value, we must first move it to a register
     * and then perform the division using that register.
     */

    if (dynamic_cast<ImmediateValue*>(div_instruction->operand.get())) {
        // Move immediate value to R10 register
        instructions.emplace_back(std::make_unique<MovInstruction>(
            div_instruction->type,
            std::move(div_instruction->operand),
            std::make_unique<Register>(RegisterName::R10)));
        div_instruction->operand = std::make_unique<Register>(RegisterName::R10);
    }
}

void FixUpInstructionsStep::fixup_movsx_instruction(MovsxInstruction* movsx_instruction,
    std::vector<std::unique_ptr<Instruction>>& instructions,
    std::unique_ptr<Instruction>& instruction)
{
    /*
     * x86-64 MOVSX (move with sign extension) instruction constraint fixups:
     * 1. MOVSX cannot use an immediate value as its source operand
     * 2. MOVSX cannot use a memory address as its destination operand
     *
     * The fixup strategy:
     * - If source is immediate: move immediate -> R10, then use R10 as source
     * - If destination is memory: use R11 as destination, then move R11 -> original destination
     */

    // Fix immediate value as source
    if (dynamic_cast<ImmediateValue*>(movsx_instruction->source.get())) {
        // Move immediate to R10 register (using LONG_WORD size for the intermediate move)
        instructions.emplace_back(std::make_unique<MovInstruction>(
            AssemblyType::LONG_WORD,
            std::move(movsx_instruction->source),
            std::make_unique<Register>(RegisterName::R10)));
        movsx_instruction->source = std::make_unique<Register>(RegisterName::R10);
    }

    // Fix memory address as destination
    std::unique_ptr<Instruction> additional_instruction = nullptr;
    bool dest_is_memory = dynamic_cast<StackAddress*>(movsx_instruction->destination.get()) || dynamic_cast<DataOperand*>(movsx_instruction->destination.get());

    if (dest_is_memory) {
        // Store the original destination for later use
        additional_instruction = std::make_unique<MovInstruction>(
            AssemblyType::QUAD_WORD,
            std::make_unique<Register>(RegisterName::R11),
            std::move(movsx_instruction->destination));
        // Use R11 as temporary destination
        movsx_instruction->destination = std::make_unique<Register>(RegisterName::R11);
    }

    // Add the fixed MOVSX instruction
    instructions.emplace_back(std::move(instruction));

    // Add the additional move instruction if needed (must come after MOVSX)
    if (additional_instruction) {
        instructions.emplace_back(std::move(additional_instruction));
    }
}

void FixUpInstructionsStep::visit(Program& node)
{
    // Process all function definitions in the program
    for (auto& def : node.definitions) {
        def->accept(*this);
    }
}
