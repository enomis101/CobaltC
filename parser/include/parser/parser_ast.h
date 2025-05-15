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

class Program;
class VariableExpression;
class AssignmentExpression;
class ConditionalExpression;
class FunctionCallExpression;
class ExpressionStatement;
class IfStatement;
class NullStatement;
class VariableDeclaration;
class FunctionDeclaration;
class Block;
class CompoundStatement;
class ForInit;
class BreakStatement;
class ContinueStatement;
class WhileStatement;
class DoWhileStatement;
class ForStatement;
class ForInitDeclaration;
class ForInitExpression;
class PrimitiveType;
class FunctionType;

// ParserVisitor interface
class ParserVisitor {
public:
    virtual void visit(Identifier& node) = 0;
    virtual void visit(UnaryExpression& node) = 0;
    virtual void visit(BinaryExpression& node) = 0;
    virtual void visit(ConstantExpression& node) = 0;
    virtual void visit(ReturnStatement& node) = 0;
    virtual void visit(Program& node) = 0;
    virtual void visit(VariableExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ConditionalExpression& node) = 0;
    virtual void visit(FunctionCallExpression& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(NullStatement& node) = 0;
    virtual void visit(VariableDeclaration& node) = 0;
    virtual void visit(FunctionDeclaration& node) = 0;
    virtual void visit(Block& node) = 0;
    virtual void visit(CompoundStatement& node) = 0;
    virtual void visit(BreakStatement& node) = 0;
    virtual void visit(ContinueStatement& node) = 0;
    virtual void visit(WhileStatement& node) = 0;
    virtual void visit(DoWhileStatement& node) = 0;
    virtual void visit(ForStatement& node) = 0;
    virtual void visit(ForInitDeclaration& node) = 0;
    virtual void visit(ForInitExpression& node) = 0;
    virtual void visit(PrimitiveType& node) {};
    virtual void visit(FunctionType& node) {};

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

class Type : public ParserAST {
public:
    virtual ~Type() = default;
};

enum class PrimitiveTypeEnum {
    INT
};

class PrimitiveType : public Type {
public:
    PrimitiveType(PrimitiveTypeEnum t)
        : type { t }
    {
    }
    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    PrimitiveTypeEnum type;
};

class FunctionType : public Type {
public:
    FunctionType(size_t pc)
        : parameters_count { pc }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    size_t parameters_count;

    // Implement operator== to compare two FunctionType objects
    bool operator==(const FunctionType& other) const
    {
        // Then check if parameters_count is equal
        return parameters_count == other.parameters_count;
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

class ConditionalExpression : public Expression {
public:
    ConditionalExpression(std::unique_ptr<Expression> cond, std::unique_ptr<Expression> t, std::unique_ptr<Expression> f)
        : condition(std::move(cond))
        , true_expression(std::move(t))
        , false_expression(std::move(f))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Expression> true_expression;
    std::unique_ptr<Expression> false_expression;
};

class FunctionCallExpression : public Expression {
public:
    FunctionCallExpression(const std::string& n, std::vector<std::unique_ptr<Expression>> args)
        : name(n)
        , arguments(std::move(args))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier name;
    std::vector<std::unique_ptr<Expression>> arguments;
};

class BlockItem : public ParserAST {
public:
    virtual ~BlockItem() = default;
};

class Block : public ParserAST {
public:
    Block(std::vector<std::unique_ptr<BlockItem>> i)
        : items(std::move(i))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<BlockItem>> items;
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

class IfStatement : public Statement {
public:
    IfStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then_stmt, std::unique_ptr<Statement> else_stmt = nullptr)
        : condition(std::move(cond))
        , then_statement(std::move(then_stmt))
        , else_statement(else_stmt ? std::optional<std::unique_ptr<Statement>>(std::move(else_stmt)) : std::nullopt)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> then_statement;
    std::optional<std::unique_ptr<Statement>> else_statement;
};

class CompoundStatement : public Statement {
public:
    CompoundStatement(std::unique_ptr<Block> b)
        : block(std::move(b))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Block> block;
};

class BreakStatement : public Statement {
public:
    BreakStatement(const std::string& l = "")
        : label { l }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier label; // used during loop-labeling stage
};

class ContinueStatement : public Statement {
public:
    ContinueStatement(const std::string& l = "")
        : label { l }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier label; // used during loop-labeling stage
};

class WhileStatement : public Statement {
public:
    WhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> b, const std::string& l = "")
        : condition { std::move(c) }
        , body { std::move(b) }
        , label { l }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    Identifier label; // used during loop-labeling stage
};

class DoWhileStatement : public Statement {
public:
    DoWhileStatement(std::unique_ptr<Expression> c, std::unique_ptr<Statement> b, const std::string& l = "")
        : condition { std::move(c) }
        , body { std::move(b) }
        , label { l }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Statement> body;
    Identifier label; // used during loop-labeling stage
};

class ForStatement : public Statement {
public:
    ForStatement(std::unique_ptr<ForInit> i, std::unique_ptr<Expression> c, std::unique_ptr<Expression> p, std::unique_ptr<Statement> b, const std::string& l = "")
        : init { std::move(i) }
        , condition { c ? std::optional<std::unique_ptr<Expression>>(std::move(c)) : std::nullopt }
        , post { p ? std::optional<std::unique_ptr<Expression>>(std::move(p)) : std::nullopt }
        , body { std::move(b) }
        , label { l }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<ForInit> init;
    std::optional<std::unique_ptr<Expression>> condition;
    std::optional<std::unique_ptr<Expression>> post;
    std::unique_ptr<Statement> body;
    Identifier label; // used during loop-labeling stage
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

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(const std::string& n, std::vector<Identifier> p, std::unique_ptr<Block> b)
        : name(n)
        , params(p)
        , body(b != nullptr ? std::optional<std::unique_ptr<Block>>(std::move(b)) : std::nullopt)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier name;
    std::vector<Identifier> params;
    std::optional<std::unique_ptr<Block>> body;
};

class ForInit : public ParserAST {
public:
    virtual ~ForInit() = default;
};

class ForInitDeclaration : public ForInit {
public:
    ForInitDeclaration(std::unique_ptr<VariableDeclaration> d)
        : declaration { std::move(d) }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<VariableDeclaration> declaration;
};

class ForInitExpression : public ForInit {
public:
    ForInitExpression(std::unique_ptr<Expression> e)
        : expression { e ? std::optional<std::unique_ptr<Expression>>(std::move(e)) : std::nullopt }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::optional<std::unique_ptr<Expression>> expression;
};

class Program : public ParserAST {
public:
    Program(std::vector<std::unique_ptr<FunctionDeclaration>> funcs)
        : functions(std::move(funcs))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<FunctionDeclaration>> functions;
};

}
