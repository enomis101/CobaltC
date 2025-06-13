#pragma once
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include <memory>
#include <string>
#include <vector>

namespace backend {

class Identifier;
class ImmediateValue;
class Register;
class PseudoRegister;
class StackAddress;
class ReturnInstruction;
class MovInstruction;
class UnaryInstruction;
class BinaryInstruction;
class CmpInstruction;
class IdivInstruction;
class CdqInstruction;
class JmpInstruction;
class JmpCCInstruction;
class SetCCInstruction;
class LabelInstruction;
class FunctionDefinition;
class Program;
class PushInstruction;
class CallInstruction;
class StaticVariable;
class DataOperand;
class MovsxInstruction;

class AssemblyVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ImmediateValue& node) = 0;
    virtual void visit(Register& node) = 0;
    virtual void visit(PseudoRegister& node) = 0;
    virtual void visit(StackAddress& node) = 0;
    virtual void visit(DataOperand& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(MovsxInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
    virtual void visit(CmpInstruction& node) = 0;
    virtual void visit(IdivInstruction& node) = 0;
    virtual void visit(CdqInstruction& node) = 0;
    virtual void visit(JmpInstruction& node) = 0;
    virtual void visit(JmpCCInstruction& node) = 0;
    virtual void visit(SetCCInstruction& node) = 0;
    virtual void visit(LabelInstruction& node) = 0;
    virtual void visit(PushInstruction& node) = 0;
    virtual void visit(CallInstruction& node) = 0;
    virtual void visit(FunctionDefinition& node) = 0;
    virtual void visit(StaticVariable& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~AssemblyVisitor() = default;
};

enum class RegisterName {
    AX,
    CX,
    DX,
    DI,
    SI,
    R8,
    R9,
    R10,
    R11,
    SP
};

enum class AssemblyType {
    BYTE,      // 1-byte
    WORD,      // 2-byte
    LONG_WORD, // 4-byte
    QUAD_WORD, // 8-byte
};

// Abstract base class for all AssemblyAST nodes
class AssemblyAST {
public:
    virtual ~AssemblyAST() = default;
    virtual void accept(class AssemblyVisitor& visitor) = 0;
};

class Identifier : public AssemblyAST {
public:
    Identifier(const std::string& name)
        : name(name)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Identifier> clone() const
    {
        return std::make_unique<Identifier>(name);
    }

    std::string name;
};

enum class UnaryOperator {
    NEG,
    NOT
};

enum class BinaryOperator {
    ADD,
    SUB,
    MULT
};

enum class ConditionCode {
    E,
    NE,
    G,
    GE,
    L,
    LE
};

// Abstract class for all expressions
class Operand : public AssemblyAST {
public:
    virtual ~Operand() = default;
    virtual std::unique_ptr<Operand> clone() const = 0;
};

class ImmediateValue : public Operand {
public:
    ImmediateValue(ConstantType v)
        : value(v)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<ImmediateValue>(value);
    }

    ConstantType value;
};

class Register : public Operand {
public:
    Register(RegisterName name, AssemblyType type = AssemblyType::LONG_WORD)
        : name { name }
        , type { type }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<Register>(name, type);
    }

    RegisterName name;
    AssemblyType type;
};

class PseudoRegister : public Operand {
public:
    PseudoRegister(const std::string& id)
        : identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<PseudoRegister>(identifier.name);
    }

    Identifier identifier;
};

class StackAddress : public Operand {
public:
    StackAddress(int off)
        : offset(off)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<StackAddress>(offset);
    }

    int offset;
};

class DataOperand : public Operand {
public:
    DataOperand(const std::string& id)
        : identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<DataOperand>(identifier.name);
    }

    Identifier identifier;
};

class Instruction : public AssemblyAST {
public:
    virtual ~Instruction() = default;
    virtual std::unique_ptr<Instruction> clone() const = 0;
};

class ReturnInstruction : public Instruction {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<ReturnInstruction>();
    }
};

class MovInstruction : public Instruction {
public:
    MovInstruction(AssemblyType type, std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
        : type(type)
        , source(std::move(src))
        , destination(std::move(dst))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<MovInstruction>(type,
            source->clone(),
            destination->clone());
    }

    AssemblyType type;
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class MovsxInstruction : public Instruction {
public:
    MovsxInstruction(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
        : source(std::move(src))
        , destination(std::move(dst))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<MovsxInstruction>(
            source->clone(),
            destination->clone());
    }

    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class UnaryInstruction : public Instruction {
public:
    UnaryInstruction(UnaryOperator unary_operator, AssemblyType type, std::unique_ptr<Operand> operand)
        : unary_operator(unary_operator)
        , type(type)
        , operand(std::move(operand))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<UnaryInstruction>(
            unary_operator,
            type,
            operand->clone());
    }

    UnaryOperator unary_operator;
    AssemblyType type;
    std::unique_ptr<Operand> operand;
};

class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(BinaryOperator binary_operator, AssemblyType type, std::unique_ptr<Operand> source, std::unique_ptr<Operand> destination)
        : binary_operator(binary_operator)
        , type(type)
        , source(std::move(source))
        , destination(std::move(destination))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<BinaryInstruction>(
            binary_operator,
            type,
            source->clone(),
            destination->clone());
    }

    BinaryOperator binary_operator;
    AssemblyType type;
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class CmpInstruction : public Instruction {
public:
    CmpInstruction(AssemblyType type, std::unique_ptr<Operand> source, std::unique_ptr<Operand> destination)
        : type(type)
        , source(std::move(source))
        , destination(std::move(destination))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<CmpInstruction>(
            type,
            source->clone(),
            destination->clone());
    }
    AssemblyType type;
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class IdivInstruction : public Instruction {
public:
    IdivInstruction(AssemblyType type, std::unique_ptr<Operand> op)
        : type(type)
        , operand(std::move(op))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<IdivInstruction>(
            type, operand->clone());
    }

    AssemblyType type;
    std::unique_ptr<Operand> operand;
};

class CdqInstruction : public Instruction {
public:
    CdqInstruction(AssemblyType type)
        : type(type)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<CdqInstruction>(type);
    }

    AssemblyType type;
};

class JmpInstruction : public Instruction {
public:
    JmpInstruction(const std::string& id)
        : identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<JmpInstruction>(
            identifier.name);
    }

    Identifier identifier;
};

class JmpCCInstruction : public Instruction {
public:
    JmpCCInstruction(ConditionCode cc, const std::string& id)
        : condition_code { cc }
        , identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<JmpCCInstruction>(
            condition_code,
            identifier.name);
    }

    ConditionCode condition_code;
    Identifier identifier;
};

class SetCCInstruction : public Instruction {
public:
    SetCCInstruction(ConditionCode cc, std::unique_ptr<Operand> dst)
        : condition_code(cc)
        , destination(std::move(dst))
    {
        if (Register* reg = dynamic_cast<Register*>(destination.get())) {
            reg->type = AssemblyType::BYTE;
        }
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<SetCCInstruction>(
            condition_code,
            destination->clone());
    }

    ConditionCode condition_code;
    std::unique_ptr<Operand> destination;
};

class LabelInstruction : public Instruction {
public:
    LabelInstruction(const std::string& id)
        : identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<LabelInstruction>(
            identifier.name);
    }

    Identifier identifier;
};

class PushInstruction : public Instruction {
public:
    PushInstruction(std::unique_ptr<Operand> dst)
        : destination(std::move(dst))
    {
        if (Register* reg = dynamic_cast<Register*>(destination.get())) {
            reg->type = AssemblyType::QUAD_WORD;
        }
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<PushInstruction>(
            destination->clone());
    }

    std::unique_ptr<Operand> destination;
};

class CallInstruction : public Instruction {
public:
    CallInstruction(const std::string& id)
        : identifier { id }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<CallInstruction>(
            identifier.name);
    }

    Identifier identifier;
};

class TopLevel : public AssemblyAST {
public:
    virtual ~TopLevel() = default;
};

class FunctionDefinition : public TopLevel {
public:
    FunctionDefinition(const std::string& n, bool glbl, std::vector<std::unique_ptr<Instruction>> i)
        : name { n }
        , global { glbl }
        , instructions(std::move(i))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<TopLevel> clone() const
    {
        std::vector<std::unique_ptr<Instruction>> cloned_instructions;
        cloned_instructions.reserve(instructions.size());

        for (const auto& instruction : instructions) {
            cloned_instructions.push_back(instruction->clone());
        }

        return std::make_unique<FunctionDefinition>(name.name, global, std::move(cloned_instructions));
    }

    Identifier name;
    bool global;
    std::vector<std::unique_ptr<Instruction>> instructions;
};

class StaticVariable : public TopLevel {
public:
    StaticVariable(const std::string& n, bool glbl, size_t alignment, StaticInitialValueType static_init)
        : name { n }
        , global { glbl }
        , alignment(alignment)
        , static_init(static_init)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<TopLevel> clone() const
    {
        return std::make_unique<StaticVariable>(name.name, global, alignment, static_init);
    }

    Identifier name;
    bool global;
    size_t alignment;
    StaticInitialValueType static_init;
};

class Program : public AssemblyAST {
public:
    Program(std::vector<std::unique_ptr<TopLevel>> defs)
        : definitions(std::move(defs))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<TopLevel>> definitions;
};

} // assembly namespace
