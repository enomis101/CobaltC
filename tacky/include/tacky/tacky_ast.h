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
class ComplementOperator;
class NegateOperator;
class Constant;
class TemporaryVariable;
class ReturnInstruction;
class UnaryInstruction;
class Function;
class Program;

// TackyVisitor interface
class TackyVisitor {
public:
    virtual void visit(ComplementOperator& node) = 0;
    virtual void visit(NegateOperator& node) = 0;
    virtual void visit(Constant& node) = 0;
    virtual void visit(TemporaryVariable& node) = 0;
    virtual void visit(ReturnInstruction& node) = 0;
    virtual void visit(UnaryInstruction& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~TackyVisitor() = default;
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
        :identifier{id}{}

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string identifier;
};


class Instruction : public TackyAST {
public:
    virtual ~Instruction() = default;
};

class ReturnInstruction : public Instruction {
public:
    ReturnInstruction(std::unique_ptr<Value> v)
    :value{std::move(v)}{}


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

class Function : public TackyAST {
public:
    Function(const std::string& n,  std::vector<std::unique_ptr<Instruction>> b)
        : name{n}
        , body(std::move(b))
    {
    }

    void accept(TackyVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
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
