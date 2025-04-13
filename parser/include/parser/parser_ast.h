#pragma once
#include <memory>
#include <string>
#include <vector>

namespace parser {

// Abstract base class for all AST nodes
class AST {
public:
    virtual ~AST() = default;
    virtual void accept(class Visitor& visitor) = 0;
};

// Forward declaration of node types
class Identifier;
class ConstantExpression;
class ReturnStatement;
class Function;
class Program;

// Visitor interface
class Visitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(ConstantExpression& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(Function& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual ~Visitor() = default;
};

// Abstract class for all expressions
class Expression : public AST {
public:
    virtual ~Expression() = default;
};

class ConstantExpression : public Expression {
public:
    ConstantExpression(int value) : value(value) {}
    
    void accept(Visitor& visitor) override
    {
        visitor.visit(*this);
    }

    int value;
};

class Identifier : public AST {
public:
    Identifier(const std::string& name) : name(name) {}
    
    void accept(Visitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
};

class Statement : public AST {
public:
    virtual ~Statement() = default;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(std::unique_ptr<Expression> expr) : expression(std::move(expr)) {}
    
    void accept(Visitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class FunctionDefinition : public AST {
public:
    virtual ~FunctionDefinition() = default;
};

class Function : public FunctionDefinition {
public:
    Function(std::unique_ptr<Identifier> name, std::unique_ptr<Statement> body) 
        : name(std::move(name)), body(std::move(body)) {}
    
    void accept(Visitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Identifier> name;
    std::unique_ptr<Statement> body;
};

class Program : public AST {
public:
    Program(std::unique_ptr<FunctionDefinition> func) : function(std::move(func)) {}
    
    void accept(Visitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<FunctionDefinition> function;
};

}