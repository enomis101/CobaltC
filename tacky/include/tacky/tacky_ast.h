#pragma once
#include <memory>
#include <string>
#include <vector>

namespace tacky {

// Abstract base class for all TackyAST nodes
class TackyAST {
public:
    virtual ~TackyAST() = default;
    virtual void accept(class TackyVisitor& visitor) = 0;
};

// Forward declaration of node types
class Identifier;
class Constant;
class TemporaryVariable;
class ReturnInstruction;
class UnaryInstruction;
class BinaryInstruction;
class CopyInstruction;
class JumpInstruction;
class JumpIfZeroInstruction;
class JumpIfNotZeroInstruction;
class LabelInstruction;
class Function;
class Program;

// TackyVisitor interface
class TackyVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(Constant& node) = 0;
    virtual void visit(TemporaryVariable& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
    virtual void visit(CopyInstruction& node) = 0;
    virtual void visit(JumpInstruction& node) = 0;
    virtual void visit(JumpIfZeroInstruction& node) = 0;
    virtual void visit(JumpIfNotZeroInstruction& node) = 0;
    virtual void visit(LabelInstruction& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~TackyVisitor() = default;
};

class Identifier : public TackyAST {
public:
    explicit Identifier(const std::string& name)
        : name(name)
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
};

enum class UnaryOperator {
    COMPLEMENT,
    NEGATE,
    NOT
};

enum class BinaryOperator {
    ADD,
    SUBTRACT,
    MULTIPLY,
    DIVIDE,
    REMAINDER,
    AND,
    OR,
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_OR_EQUAL,
    GREATER_THAN,
    GREATER_OR_EQUAL
};

class Value : public TackyAST {
public:
    virtual ~Value() = default;
};

class Constant : public Value {
public:
    Constant(int value)
        : value(value)
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    int value;
};

class TemporaryVariable : public Value {
public:
    TemporaryVariable(const std::string& id)
        : identifier { id }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
};

class Instruction : public TackyAST {
public:
    virtual ~Instruction() = default;
};

class ReturnInstruction : public Instruction {
public:
    ReturnInstruction(std::unique_ptr<Value> v)
        : value { std::move(v) }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Value> value;
};

class UnaryInstruction : public Instruction {
public:
    UnaryInstruction(UnaryOperator op, std::unique_ptr<Value> src, std::unique_ptr<Value> dst)
        : unary_operator(op)
        , source(std::move(src))
        , destination(std::move(dst))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    UnaryOperator unary_operator;
    std::unique_ptr<Value> source;
    std::unique_ptr<Value> destination;
};

class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(BinaryOperator op, std::unique_ptr<Value> src1, std::unique_ptr<Value> src2, std::unique_ptr<Value> dst)
        : binary_operator(op)
        , source1(std::move(src1))
        , source2(std::move(src2))
        , destination(std::move(dst))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    BinaryOperator binary_operator;
    std::unique_ptr<Value> source1;
    std::unique_ptr<Value> source2;
    std::unique_ptr<Value> destination;
};

class CopyInstruction : public Instruction {
public:
    CopyInstruction(std::unique_ptr<Value> src, std::unique_ptr<Value> dst)
        : source(std::move(src))
        , destination(std::move(dst))
    {
    }
    CopyInstruction(int src, std::unique_ptr<Value> dst)
        : source(std::make_unique<Constant>(src))
        , destination(std::move(dst))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Value> source;
    std::unique_ptr<Value> destination;
};

class JumpInstruction : public Instruction {
public:
    JumpInstruction(const std::string& id)
        : identifier { id }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
};

class JumpIfZeroInstruction : public Instruction {
public:
    JumpIfZeroInstruction(std::unique_ptr<Value> cond, const std::string& id)
        : condition { std::move(cond) }
        , identifier { id }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Value> condition;
    Identifier identifier;
};

class JumpIfNotZeroInstruction : public Instruction {
public:
    JumpIfNotZeroInstruction(std::unique_ptr<Value> cond, const std::string& id)
        : condition { std::move(cond) }
        , identifier { id }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Value> condition;
    Identifier identifier;
};

class LabelInstruction : public Instruction {
public:
    LabelInstruction(const std::string& id)
        : identifier { id }
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
};

class Function : public TackyAST {
public:
    Function(const std::string& n, std::vector<std::unique_ptr<Instruction>> b)
        : name { n }
        , body(std::move(b))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier name;
    std::vector<std::unique_ptr<Instruction>> body;
};

class Program : public TackyAST {
public:
    Program(std::unique_ptr<Function> func)
        : function(std::move(func))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Function> function;
};

}
