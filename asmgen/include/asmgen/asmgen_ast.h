#pragma once
#include <memory>
#include <string>
#include <vector>

namespace asmgen {

// Abstract base class for all AsmGenAST nodes
class AsmGenAST {
public:
    virtual ~AsmGenAST() = default;
    virtual void accept(class AsmGenVisitor& visitor) = 0;
};

// Forward declaration of node types
class Identifier;
class ImmediateValue;
class Register;
class ReturnInstruction;
class MovInstruction;
class Function;
class Program;

// AsmGenVisitor interface
class AsmGenVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ImmediateValue& node) = 0;
    virtual void visit(Register& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(MovInstruction& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~AsmGenVisitor() = default;
};

// Abstract class for all expressions
class Operand : public AsmGenAST {
public:
    virtual ~Operand() = default;
};

class ImmediateValue : public Operand {
public:
    ImmediateValue(int value)
        : value(value)
    {
    }

    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    int value;
};

class Register : public Operand {
public:
    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class Identifier : public AsmGenAST {
public:
    Identifier(const std::string& name)
        : name(name)
    {
    }

    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
};

class Instruction : public AsmGenAST {
public:
    virtual ~Instruction() = default;
};

class ReturnInstruction : public Instruction {
public:

    void accept(AsmGenVisitor& visitor) override
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
    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Operand> src;
    std::unique_ptr<Operand> dst;

};

class Function : public AsmGenAST {
public:
    Function(std::unique_ptr<Identifier> n,  std::vector<std::unique_ptr<Instruction>> i)
        : name(std::move(n))
        , instructions(std::move(i))
    {
    }

    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Identifier> name;
    std::vector<std::unique_ptr<Instruction>> instructions;
};

class Program : public AsmGenAST {
public:
    Program(std::unique_ptr<Function> func)
        : function(std::move(func))
    {
    }

    void accept(AsmGenVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Function> function;
};

}
