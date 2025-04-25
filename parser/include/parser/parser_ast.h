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
class AddOperator;
class SubtractOperator;
class MultiplyOperator;
class DivideOperator;
class RemainderOperator;
class UnaryExpression;
class BinaryExpression;
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
    virtual void visit(AddOperator& node) = 0;
    virtual void visit(SubtractOperator& node) = 0;
    virtual void visit(MultiplyOperator& node) = 0;
    virtual void visit(DivideOperator& node) = 0;
    virtual void visit(RemainderOperator& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(ConstantExpression& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~ParserVisitor() = default;
};

// UNARY_OPERATOR
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

// BINARY_OPERATOR
class BinaryOperator : public ParserAST {
public:
    virtual ~BinaryOperator() = default;
};

class AddOperator : public BinaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class SubtractOperator : public BinaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class MultiplyOperator : public BinaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class DivideOperator : public BinaryOperator {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class RemainderOperator : public BinaryOperator {
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

class BinaryExpression : public Expression {
public:
    BinaryExpression(std::unique_ptr<BinaryOperator> op, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : binary_operator(std::move(op))
        , left_expression(std::move(l))
        , right_expression(std::move(r))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<BinaryOperator> binary_operator;
    std::unique_ptr<Expression> left_expression;
    std::unique_ptr<Expression> right_expression;
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

class Function : public ParserAST {
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
    Program(std::unique_ptr<Function> func)
        : function(std::move(func))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Function> function;
};

}
