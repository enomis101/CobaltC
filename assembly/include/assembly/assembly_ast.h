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
class NotOperator;
class NegOperator;
class AddOperator;
class SubOperator;
class MultOperator;
class ReturnInstruction;
class MovInstruction;
class UnaryInstruction;
class BinaryInstruction;
class IdivInstruction;
class CdqInstruction;
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
    virtual void visit(NotOperator& node) = 0;
    virtual void visit(NegOperator& node) = 0;
    virtual void visit(AddOperator& node) = 0;
    virtual void visit(SubOperator& node) = 0;
    virtual void visit(MultOperator& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
    virtual void visit(IdivInstruction& node) = 0;
    virtual void visit(CdqInstruction& node) = 0;
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

class UnaryOperator : public AssemblyAST {
public:
    virtual ~UnaryOperator() = default;
    virtual std::unique_ptr<UnaryOperator> clone() const = 0;
};

class NotOperator : public UnaryOperator {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<UnaryOperator> clone() const override
    {
        return std::make_unique<NotOperator>();
    }
};

class NegOperator : public UnaryOperator {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<UnaryOperator> clone() const override
    {
        return std::make_unique<NegOperator>();
    }
};

class BinaryOperator : public AssemblyAST {
public:
    virtual ~BinaryOperator() = default;
    virtual std::unique_ptr<BinaryOperator> clone() const = 0;
};

class AddOperator : public BinaryOperator {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<BinaryOperator> clone() const override
    {
        return std::make_unique<AddOperator>();
    }
};

class SubOperator : public BinaryOperator {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<BinaryOperator> clone() const override
    {
        return std::make_unique<SubOperator>();
    }
};

class MultOperator : public BinaryOperator {
public:
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<BinaryOperator> clone() const override
    {
        return std::make_unique<MultOperator>();
    }
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
    Register(RegisterName r)
        : reg { r }
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    
    std::unique_ptr<Operand> clone() const override
    {
        return std::make_unique<Register>(reg);
    }

    RegisterName reg;
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
            destination->clone()
        );
    }
    
    std::unique_ptr<Operand> source;
    std::unique_ptr<Operand> destination;
};

class UnaryInstruction : public Instruction {
public:
    UnaryInstruction(std::unique_ptr<UnaryOperator> u_op, std::unique_ptr<Operand> op)
        : unary_operator(std::move(u_op))
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
            unary_operator->clone(),
            operand->clone()
        );
    }

    std::unique_ptr<UnaryOperator> unary_operator;
    std::unique_ptr<Operand> operand;
};

class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(std::unique_ptr<BinaryOperator> b_op, std::unique_ptr<Operand> src, std::unique_ptr<Operand> dst)
        : binary_operator(std::move(b_op))
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
            binary_operator->clone(),
            source->clone(),
            destination->clone()
        );
    }

    std::unique_ptr<BinaryOperator> binary_operator;
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
            operand->clone()
        );
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