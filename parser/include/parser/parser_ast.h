#pragma once
#include <memory>
#include <string>
#include <vector>

namespace parser {

// Abstract base class for all ParserAST nodes
class ParserAST {
public:
    virtual ~ParserAST() = default;
    virtual void accept(class ParserVisitor& visitor) = 0;
};

// Forward declaration of node types
class Identifier;
class ComplementOperator;
class NegateOperator;
class UnaryExpression;
class ConstantExpression;
class ReturnStatement;
class Function;
class Program;

// ParserVisitor interface
class ParserVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ComplementOperator& node) = 0;
    virtual void visit(NegateOperator& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(ConstantExpression& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~ParserVisitor() = default;
};

class UnaryOperator : public ParserAST {
public:
    virtual ~UnaryOperator() = default;
};

class ComplementOperator : public UnaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class NegateOperator : public UnaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

// Abstract class for all expressions
class Expression : public ParserAST {
public:
    virtual ~Expression() = default;
};

class ConstantExpression : public Expression {
public:
    ConstantExpression(int value)
        : value(value)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    int value;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(std::unique_ptr<UnaryOperator> op, std::unique_ptr<Expression> expr)
        : unary_operator(std::move(op))
        , expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<UnaryOperator> unary_operator;
    std::unique_ptr<Expression> expression;
};

class Identifier : public ParserAST {
public:
    Identifier(const std::string& name)
        : name(name)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
};

class Statement : public ParserAST {
public:
    virtual ~Statement() = default;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(std::unique_ptr<Expression> expr)
        : expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class FunctionDefinition : public ParserAST {
public:
    virtual ~FunctionDefinition() = default;
};

class Function : public FunctionDefinition {
public:
    Function(std::unique_ptr<Identifier> name, std::unique_ptr<Statement> body)
        : name(std::move(name))
        , body(std::move(body))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Statement> body;
};

class Program : public ParserAST {
public:
    Program(std::unique_ptr<FunctionDefinition> func)
        : function(std::move(func))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<FunctionDefinition> function;
};

}
