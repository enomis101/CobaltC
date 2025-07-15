#pragma once
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace backend {

class Identifier;
class ImmediateValue;
class Register;
class PseudoRegister;
class MemoryAddress;
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
class CommentInstruction;
class MovZeroExtendInstruction;
class DivInstruction;
class StaticConstant;
class Cvttsd2siInstruction;
class Cvtsi2sdInstruction;
class LeaInstruction;
class IndexedAddress;
class PseudoMemory;

class AssemblyVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ImmediateValue& node) = 0;
    virtual void visit(Register& node) = 0;
    virtual void visit(PseudoRegister& node) = 0;
    virtual void visit(MemoryAddress& node) = 0;
    virtual void visit(IndexedAddress& node) = 0;
    virtual void visit(PseudoMemory& node) = 0;
    virtual void visit(DataOperand& node) = 0;
    virtual void visit(CommentInstruction& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(MovsxInstruction& node) = 0;
    virtual void visit(MovZeroExtendInstruction& node) = 0;
    virtual void visit(LeaInstruction& node) = 0;
    virtual void visit(Cvttsd2siInstruction& node) = 0;
    virtual void visit(Cvtsi2sdInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
    virtual void visit(CmpInstruction& node) = 0;
    virtual void visit(IdivInstruction& node) = 0;
    virtual void visit(DivInstruction& node) = 0;
    virtual void visit(CdqInstruction& node) = 0;
    virtual void visit(JmpInstruction& node) = 0;
    virtual void visit(JmpCCInstruction& node) = 0;
    virtual void visit(SetCCInstruction& node) = 0;
    virtual void visit(LabelInstruction& node) = 0;
    virtual void visit(PushInstruction& node) = 0;
    virtual void visit(CallInstruction& node) = 0;
    virtual void visit(FunctionDefinition& node) = 0;
    virtual void visit(StaticVariable& node) = 0;
    virtual void visit(StaticConstant& node) = 0;
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
    SP,
    BP,
    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7,
    XMM14,
    XMM15,
    MAX_REG
};

class AssemblyType {
public:
    enum Type {
        BYTE,
        WORD,
        LONG_WORD,
        QUAD_WORD,
        DOUBLE,
        BYTE_ARRAY,
        NONE
    };

    AssemblyType()
        : m_type { Type::NONE }
        , m_size { 0 }
        , m_alignment { 0 }
    {
    }
    AssemblyType(Type type)
        : m_type { type }
        , m_size { 0 }
        , m_alignment { 0 }
    {
    }

    explicit AssemblyType(Type type, size_t size, size_t alignment)
        : m_type { type }
        , m_size { size }
        , m_alignment { alignment }
    {
    }

    operator Type() const { return m_type; }

    Type type() const { return m_type; }

    size_t size() const
    {
        switch (m_type) {
        case BYTE:
            return 1;
        case WORD:
            return 2;
        case LONG_WORD:
            return 4;
        case QUAD_WORD:
            return 8;
        case DOUBLE:
            return 8;
        case NONE:
            return 0;
        case BYTE_ARRAY:
            return m_size;
        }
        return 0;
    }

    size_t alignment() const
    {
        switch (m_type) {
        case BYTE:
            return 1;
        case WORD:
            return 2;
        case LONG_WORD:
            return 4;
        case QUAD_WORD:
            return 8;
        case DOUBLE:
            return 8;
        case NONE:
            return 0;
        case BYTE_ARRAY:
            return m_alignment;
        }
        return 0;
    }

    bool is_byte_array() const
    {
        return m_type == BYTE_ARRAY;
    }

    bool operator==(Type other) const { return m_type == other; }
    bool operator!=(Type other) const { return m_type != other; }

private:
    Type m_type;
    size_t m_size;
    size_t m_alignment;
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
    NOT,
    SHR
};

enum class BinaryOperator {
    ADD,
    SUB,
    MULT,
    DIV_DOUBLE,
    AND,
    OR,
    XOR
};

enum class ConditionCode {
    E,
    NE,
    G,
    GE,
    L,
    LE,
    A,
    AE,
    B,
    BE,
    NONE
};

// Abstract class for all expressions
class Operand : public AssemblyAST {
public:
    virtual ~Operand() = default;
    virtual std::unique_ptr<Operand> clone() const = 0;

    virtual bool is_memory() const { return false; }
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
        if (name > RegisterName::MAX_REG)
            assert(false);
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

class MemoryAddress : public Operand {
public:
    MemoryAddress(RegisterName base_register_name, long offset)
        : base_register(std::make_unique<Register>(base_register_name, AssemblyType::QUAD_WORD))
        , offset(offset)
    {
    }

    MemoryAddress(std::unique_ptr<Register> base_register, int offset)
        : base_register(std::move(base_register))
        , offset(offset)
    {
        this->base_register->type = AssemblyType::QUAD_WORD;
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        auto reg = base_register->clone();
        return std::make_unique<MemoryAddress>(std::unique_ptr<Register>(dynamic_cast<Register*>(reg.release())), offset);
    }

    bool is_memory() const override { return true; }

    std::unique_ptr<Register> base_register;
    long offset;
};

class IndexedAddress : public Operand {
public:
    IndexedAddress(RegisterName base_register_name, RegisterName index_register_name, int offset)
        : base_register(std::make_unique<Register>(base_register_name, AssemblyType::QUAD_WORD))
        , index_register(std::make_unique<Register>(index_register_name, AssemblyType::QUAD_WORD))
        , offset(offset)
    {
    }

    IndexedAddress(std::unique_ptr<Register> base_register, std::unique_ptr<Register> index_register, int offset)
        : base_register(std::move(base_register))
        , index_register(std::move(index_register))
        , offset(offset)
    {
        this->base_register->type = AssemblyType::QUAD_WORD;
        this->index_register->type = AssemblyType::QUAD_WORD;
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        auto base_reg = std::unique_ptr<Register>(dynamic_cast<Register*>(base_register->clone().release()));
        auto index_reg = std::unique_ptr<Register>(dynamic_cast<Register*>(index_register->clone().release()));
        return std::make_unique<IndexedAddress>(std::move(base_reg), std::move(index_reg), offset);
    }

    bool is_memory() const override { return true; }

    std::unique_ptr<Register> base_register;
    std::unique_ptr<Register> index_register;
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

    bool is_memory() const override { return true; }

    Identifier identifier;
};

class PseudoMemory : public Operand {
public:
    PseudoMemory(const std::string& id, int offset)
        : identifier { id }
        , offset(offset)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<PseudoMemory>(identifier.name, offset);
    }

    Identifier identifier;
    int offset;
};

class Instruction : public AssemblyAST {
public:
    virtual ~Instruction() = default;
    virtual std::unique_ptr<Instruction> clone() const = 0;

protected:
    void check_and_replace_register_type(AssemblyType type, Operand* operand)
    {
        if (auto reg = dynamic_cast<Register*>(operand)) {
            reg->type = type;
        }
    }
};

class CommentInstruction : public Instruction {
public:
    CommentInstruction(const std::string& message)
        : message(message)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<CommentInstruction>(message);
    }

    std::string message;
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
        switch (type) {
        case AssemblyType::LONG_WORD:
        case AssemblyType::QUAD_WORD:
        case AssemblyType::DOUBLE:
            break;
        case AssemblyType::BYTE_ARRAY:
            break;
        default:
            assert(false);
        }
        check_and_replace_register_type(type, this->source.get());
        check_and_replace_register_type(type, this->destination.get());
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

class MovZeroExtendInstruction : public Instruction {
public:
    MovZeroExtendInstruction(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
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
        return std::make_unique<MovZeroExtendInstruction>(
            source->clone(),
            destination->clone());
    }

    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class LeaInstruction : public Instruction {
public:
    LeaInstruction(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
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
        return std::make_unique<LeaInstruction>(
            source->clone(),
            destination->clone());
    }

    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class Cvttsd2siInstruction : public Instruction {
public:
    Cvttsd2siInstruction(AssemblyType type, std::unique_ptr<Operand> source, std::unique_ptr<Operand> destination)
        : type(type)
        , source(std::move(source))
        , destination(std::move(destination))
    {
        check_and_replace_register_type(type, this->source.get());
        check_and_replace_register_type(type, this->destination.get());
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<Cvttsd2siInstruction>(
            type,
            source->clone(),
            destination->clone());
    }

    AssemblyType type;
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class Cvtsi2sdInstruction : public Instruction {
public:
    Cvtsi2sdInstruction(AssemblyType type, std::unique_ptr<Operand> source, std::unique_ptr<Operand> destination)
        : type(type)
        , source(std::move(source))
        , destination(std::move(destination))
    {
        check_and_replace_register_type(type, this->source.get());
        check_and_replace_register_type(type, this->destination.get());
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<Cvtsi2sdInstruction>(
            type,
            source->clone(),
            destination->clone());
    }

    AssemblyType type;
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
        check_and_replace_register_type(type, this->operand.get());
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
        check_and_replace_register_type(type, this->source.get());
        check_and_replace_register_type(type, this->destination.get());
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
        check_and_replace_register_type(type, this->source.get());
        check_and_replace_register_type(type, this->destination.get());
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
        check_and_replace_register_type(type, this->operand.get());
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

class DivInstruction : public Instruction {
public:
    DivInstruction(AssemblyType type, std::unique_ptr<Operand> op)
        : type(type)
        , operand(std::move(op))
    {
        check_and_replace_register_type(type, this->operand.get());
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<DivInstruction>(
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
        check_and_replace_register_type(AssemblyType::BYTE, this->destination.get());
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
        check_and_replace_register_type(AssemblyType::QUAD_WORD, this->destination.get());
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
    StaticVariable(const std::string& name, bool global, size_t alignment, StaticInitialValue static_init)
        : name { name }
        , global { global }
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
    StaticInitialValue static_init;
};

class StaticConstant : public TopLevel {
public:
    StaticConstant(const std::string& name, size_t alignment, StaticInitialValue static_init)
        : name { name }
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
        return std::make_unique<StaticConstant>(name.name, alignment, static_init);
    }

    Identifier name;
    size_t alignment;
    StaticInitialValue static_init;
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
