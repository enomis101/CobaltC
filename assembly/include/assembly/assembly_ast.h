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
class ReturnInstruction;
class MovInstruction;
class UnaryInstruction;
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
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(AllocateStackInstruction& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~AssemblyVisitor() = default;
};

enum class RegisterName{
    AX,
    R10
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

    std::string name;
};

    

class UnaryOperator : public AssemblyAST {
public:
    virtual ~UnaryOperator() = default;
};

class NotOperator : public UnaryOperator {
public:

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class NegOperator : public UnaryOperator {
public:

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

// Abstract class for all expressions
class Operand : public AssemblyAST {
public:
    virtual ~Operand() = default;
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

    int value;
};

class Register : public Operand {
public:
    Register(RegisterName r)
        : reg{r}{}

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    RegisterName reg;
};

class PseudoRegister : public Operand {
public:
    PseudoRegister(std::unique_ptr<Identifier> id)
        : identifier(std::move(id)){}

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Identifier> identifier;
};

class StackAddress : public Operand {
public:
    StackAddress(int off)
        : offset(off){}

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    int offset;
};
    

class Instruction : public AssemblyAST {
public:
    virtual ~Instruction() = default;
};

class ReturnInstruction : public Instruction {
public:

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class MovInstruction : public Instruction {
public:

    MovInstruction(std::unique_ptr<Operand> s, std::unique_ptr<Operand> d)
        : src(std::move(s))
        , dst(std::move(d))
    {
    }
    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Operand> src;
    std::unique_ptr<Operand> dst;

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

    std::unique_ptr<UnaryOperator> unary_operator;
    std::unique_ptr<Operand> operand;

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

    int value;
};

class Function : public AssemblyAST {
public:
    Function(std::unique_ptr<Identifier> n,  std::vector<std::unique_ptr<Instruction>> i)
        : name(std::move(n))
        , instructions(std::move(i))
    {
    }

    void accept(AssemblyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Identifier> name;
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
    std::unique_ptr<Function> function;
};

}   //assembly namespace
