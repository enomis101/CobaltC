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
class ComplementOperator;
class NegateOperator;
class AddOperator;
class SubtractOperator;
class MultiplyOperator;
class DivideOperator;
class RemainderOperator;
class Constant;
class TemporaryVariable;
class ReturnInstruction;
class UnaryInstruction;
class BinaryInstruction;
class Function;
class Program;

// TackyVisitor interface
class TackyVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ComplementOperator& node) = 0;
    virtual void visit(NegateOperator& node) = 0;
    virtual void visit(AddOperator& node) = 0;
    virtual void visit(SubtractOperator& node) = 0;
    virtual void visit(MultiplyOperator& node) = 0;
    virtual void visit(DivideOperator& node) = 0;
    virtual void visit(RemainderOperator& node) = 0;
    virtual void visit(Constant& node) = 0;
    virtual void visit(TemporaryVariable& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(BinaryInstruction& node) = 0;
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

class UnaryOperator : public TackyAST {
public:
    virtual ~UnaryOperator() = default;
};

class ComplementOperator : public UnaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class NegateOperator : public UnaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

// BINARY_OPERATOR
class BinaryOperator : public TackyAST {
public:
    virtual ~BinaryOperator() = default;
};

class AddOperator : public BinaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class SubtractOperator : public BinaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class MultiplyOperator : public BinaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class DivideOperator : public BinaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class RemainderOperator : public BinaryOperator {
public:
    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }
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
    UnaryInstruction(std::unique_ptr<UnaryOperator> op, std::unique_ptr<Value> src, std::unique_ptr<Value> dst)
        : unary_operator(std::move(op))
        , source(std::move(src))
        , destination(std::move(dst))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<UnaryOperator> unary_operator;
    std::unique_ptr<Value> source;
    std::unique_ptr<Value> destination;
};

class BinaryInstruction : public Instruction {
public:
    BinaryInstruction(std::unique_ptr<BinaryOperator> op, std::unique_ptr<Value> src1, std::unique_ptr<Value> src2, std::unique_ptr<Value> dst)
        : binary_operator(std::move(op))
        , source1(std::move(src1))
        , source2(std::move(src2))
        , destination(std::move(dst))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<BinaryOperator> binary_operator;
    std::unique_ptr<Value> source1;
    std::unique_ptr<Value> source2;
    std::unique_ptr<Value> destination;
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
