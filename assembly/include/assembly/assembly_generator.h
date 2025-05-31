#pragma once
#include "assembly/assembly_ast.h"
#include "common/data/symbol_table.h"
#include "tacky/tacky_ast.h"
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace assembly {

class AssemblyGeneratorError : public std::runtime_error {
public:
    explicit AssemblyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class PseudoRegisterReplaceStep : public AssemblyVisitor {
public:
    PseudoRegisterReplaceStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<SymbolTable> symbol_table);

    void replace();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override { }
    void visit(DataOperand& node) override { }
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override;
    void visit(UnaryInstruction& node) override;
    void visit(BinaryInstruction& node) override;
    void visit(CmpInstruction& node) override;
    void visit(IdivInstruction& node) override;
    void visit(CdqInstruction& node) override { }
    void visit(JmpInstruction& node) override { }
    void visit(JmpCCInstruction& node) override { }
    void visit(SetCCInstruction& node) override;
    void visit(LabelInstruction& node) override { }
    void visit(AllocateStackInstruction& node) override { }
    void visit(DeallocateStackInstruction& node) override { }
    void visit(PushInstruction& node) override;
    void visit(CallInstruction& node) override { }
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override { }
    void visit(Program& node) override;

    int get_offset(const std::string& name);
    void check_and_replace(std::unique_ptr<Operand>& op);

    std::shared_ptr<AssemblyAST> m_ast;
    std::unordered_map<std::string, int> m_stack_offsets;
    std::shared_ptr<SymbolTable> m_symbol_table;
};

class FixUpInstructionsStep : public AssemblyVisitor {
public:
    FixUpInstructionsStep(std::shared_ptr<AssemblyAST> ast, std::shared_ptr<SymbolTable> symbol_table);

    void fixup();

private:
    // Assembly Visitor Interface
    void visit(Identifier& node) override { }
    void visit(ImmediateValue& node) override { }
    void visit(Register& node) override { }
    void visit(PseudoRegister& node) override { }
    void visit(StackAddress& node) override { }
    void visit(DataOperand& node) override { }
    void visit(ReturnInstruction& node) override { }
    void visit(MovInstruction& node) override { }
    void visit(UnaryInstruction& node) override { }
    void visit(BinaryInstruction& node) override { }
    void visit(CmpInstruction& node) override { }
    void visit(IdivInstruction& node) override { }
    void visit(CdqInstruction& node) override { }
    void visit(JmpInstruction& node) override { }
    void visit(JmpCCInstruction& node) override { }
    void visit(SetCCInstruction& node) override { }
    void visit(LabelInstruction& node) override { }
    void visit(AllocateStackInstruction& node) override { }
    void visit(DeallocateStackInstruction& node) override { }
    void visit(PushInstruction& node) override { }
    void visit(CallInstruction& node) override { }
    void visit(FunctionDefinition& node) override;
    void visit(StaticVariable& node) override { }
    void visit(Program& node) override;

    template<typename I>
    void fixup_double_memory_address_instruction(std::unique_ptr<Instruction>& i, std::vector<std::unique_ptr<Instruction>>& instructions)
    {
        if (I* instruction = dynamic_cast<I*>(i.get())) {
            if ((dynamic_cast<StackAddress*>(instruction->source.get()) || dynamic_cast<DataOperand*>(instruction->source.get()))
                && (dynamic_cast<StackAddress*>(instruction->destination.get()) || dynamic_cast<DataOperand*>(instruction->destination.get()))) {
                instructions.emplace_back(std::make_unique<MovInstruction>(std::move(instruction->source), std::make_unique<Register>(RegisterName::R10)));
                instruction->source = std::make_unique<Register>(RegisterName::R10);
            }
            // push a copy of instruction
            instructions.emplace_back(std::move(i));
        } else {
            throw AssemblyGeneratorError("Invalid Instruction type passed to fixup_double_stack_address_instruction");
        }
    }

    std::shared_ptr<AssemblyAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;

    int round_up_to_16(int x)
    {
        return ((x + 15) / 16) * 16;
    }
};

// Generate an AssemblyAST from a TackyAST
class AssemblyGenerator {
public:
    AssemblyGenerator(std::shared_ptr<tacky::TackyAST> ast, std::shared_ptr<SymbolTable> symbol_table);

    std::shared_ptr<AssemblyAST> generate();

private:
    std::unique_ptr<Operand> transform_operand(tacky::Value& op);
    UnaryOperator transform_operator(tacky::UnaryOperator& unary_operator);
    BinaryOperator transform_operator(tacky::BinaryOperator& binary_operator);
    std::vector<std::unique_ptr<Instruction>> transform_instruction(tacky::Instruction& instruction);
    std::vector<std::unique_ptr<Instruction>> transform_unary_instruction(tacky::UnaryInstruction& unary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_binary_instruction(tacky::BinaryInstruction& binary_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_jump_instruction(tacky::Instruction& jump_instruction);
    std::vector<std::unique_ptr<Instruction>> transform_function_call_instruction(tacky::FunctionCallInstruction& function_call_instruction);
    std::unique_ptr<FunctionDefinition> transform_function(tacky::FunctionDefinition& function);
    std::unique_ptr<TopLevel> transform_top_level(tacky::TopLevel& top_level);
    std::unique_ptr<Program> transform_program(tacky::Program& program);

    bool is_relational_operator(tacky::BinaryOperator op);
    ConditionCode to_condition_code(tacky::BinaryOperator op);
    std::shared_ptr<tacky::TackyAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
    const std::vector<RegisterName> FUN_REGISTERS;
};

} // namespace assembly
