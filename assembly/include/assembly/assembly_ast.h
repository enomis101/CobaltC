#pragma once
#include <memory>
#include <string>
#include <vector>

namespace assembly {

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
class AllocateStackInstruction;
class Function;
class Program;

class AssemblyVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ImmediateValue& node) = 0;
    virtual void visit(Register& node) = 0;
    virtual void visit(PseudoRegister& node) = 0;
    virtual void visit(StackAddress& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
    virtual void visit(CmpInstruction& node) = 0;
    virtual void visit(IdivInstruction& node) = 0;
    virtual void visit(CdqInstruction& node) = 0;
    virtual void visit(JmpInstruction& node) = 0;
    virtual void visit(JmpCCInstruction& node) = 0;
    virtual void visit(SetCCInstruction& node) = 0;
    virtual void visit(LabelInstruction& node) = 0;
    virtual void visit(AllocateStackInstruction& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~AssemblyVisitor() = default;
};

enum class RegisterName {
    AX,
    DX,
    R10,
    R11
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
    ImmediateValue(int v)
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

    int value;
};

class Register : public Operand {
public:
    Register(RegisterName r, bool b = false)
        : reg { r }
        , single_byte { b }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<Register>(reg, single_byte);
    }

    RegisterName reg;
    bool single_byte; // set to true by the SetCC instruction
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
    MovInstruction(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
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
        return std::make_unique<MovInstruction>(
            source->clone(),
            destination->clone());
    }

    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class UnaryInstruction : public Instruction {
public:
    UnaryInstruction(UnaryOperator u_op, std::unique_ptr<Operand> op)
        : unary_operator(u_op)
        , operand(std::move(op))
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
            operand->clone());
    }

    UnaryOperator unary_operator;
    std::unique_ptr<Operand> operand;
};

class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(BinaryOperator b_op, std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
        : binary_operator(b_op)
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
        return std::make_unique<BinaryInstruction>(
            binary_operator,
            source->clone(),
            destination->clone());
    }

    BinaryOperator binary_operator;
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class CmpInstruction : public Instruction {
public:
    CmpInstruction(std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
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
        return std::make_unique<CmpInstruction>(
            source->clone(),
            destination->clone());
    }

    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class IdivInstruction : public Instruction {
public:
    IdivInstruction(std::unique_ptr<Operand> op)
        : operand(std::move(op))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<IdivInstruction>(
            operand->clone());
    }

    std::unique_ptr<Operand> operand;
};

class CdqInstruction : public Instruction {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<CdqInstruction>();
    }
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
            reg->single_byte = true;
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

class AllocateStackInstruction : public Instruction {
public:
    AllocateStackInstruction(int value)
        : value(value)
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Instruction> clone() const override
    {
        return std::make_unique<AllocateStackInstruction>(value);
    }

    int value;
};

class Function : public AssemblyAST {
public:
    Function(const std::string& n, std::vector<std::unique_ptr<Instruction>> i)
        : name { n }
        , instructions(std::move(i))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Function> clone() const
    {
        std::vector<std::unique_ptr<Instruction>> cloned_instructions;
        cloned_instructions.reserve(instructions.size());

        for (const auto& instruction : instructions) {
            cloned_instructions.push_back(instruction->clone());
        }

        return std::make_unique<Function>(name.name, std::move(cloned_instructions));
    }

    Identifier name;
    std::vector<std::unique_ptr<Instruction>> instructions;
};

class Program : public AssemblyAST {
public:
    Program(std::unique_ptr<Function> func)
        : function(std::move(func))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Program> clone() const
    {
        return std::make_unique<Program>(function->clone());
    }

    std::unique_ptr<Function> function;
};

} // assembly namespace
