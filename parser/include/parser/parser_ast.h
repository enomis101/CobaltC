#pragma once
#include <memory>
#include <optional>
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
class UnaryExpression;
class BinaryExpression;
class ConstantExpression;
class ReturnStatement;
class Function;
class Program;
class VariableExpression;
class AssignmentExpression;
class ExpressionStatement;
class NullStatement;
class VariableDeclaration;

// ParserVisitor interface
class ParserVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(ConstantExpression& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual void visit(VariableExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(NullStatement& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;

    virtual ~ParserVisitor() = default;
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

class VariableExpression : public Expression {
public:
    VariableExpression(const std::string& id)
        : identifier { id }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(UnaryOperator op, std::unique_ptr<Expression> expr)
        : unary_operator(op)
        , expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    UnaryOperator unary_operator;
    std::unique_ptr<Expression> expression;
};

class BinaryExpression : public Expression {
public:
    BinaryExpression(BinaryOperator op, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : binary_operator(op)
        , left_expression(std::move(l))
        , right_expression(std::move(r))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    BinaryOperator binary_operator;
    std::unique_ptr<Expression> left_expression;
    std::unique_ptr<Expression> right_expression;
};

class AssignmentExpression : public Expression {
public:
    AssignmentExpression(std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : left_expression(std::move(l))
        , right_expression(std::move(r))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Expression> left_expression;
    std::unique_ptr<Expression> right_expression;
};



class BlockItem : public ParserAST {
public:
    virtual ~BlockItem() = default;
};

class Statement : public BlockItem {
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

class ExpressionStatement : public Statement {
public:
    ExpressionStatement(std::unique_ptr<Expression> expr)
        : expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class NullStatement : public Statement {
public:
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};

class Declaration : public BlockItem {
public:
    virtual ~Declaration() = default;
};

class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(const std::string& id, std::unique_ptr<Expression> expr = nullptr)
        : identifier { id }
        , expression(expr ? std::optional<std::unique_ptr<Expression>>(std::move(expr)) : std::nullopt)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
    std::optional<std::unique_ptr<Expression>> expression;
};

class Function : public ParserAST {
public:
    Function(std::unique_ptr<Identifier> name, std::vector<std::unique_ptr<BlockItem>> body)
        : name(std::move(name))
        , body(std::move(body))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Identifier> name;
    std::vector<std::unique_ptr<BlockItem>> body;
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
