#pragma once
#include "common/data/source_location.h"
#include "common/data/type.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace parser {

// Abstract base class for all ParserAST nodes
class ParserAST {
public:
    explicit ParserAST(SourceLocationIndex loc)
        : source_location(loc)
    {
    }
    virtual ~ParserAST() = default;
    virtual void accept(class ParserVisitor& visitor) = 0;

    SourceLocationIndex source_location;
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
class CastExpression;
class DereferenceExpression;
class AddressOfExpression;
class SubscriptExpression;
class SingleInitializer;
class CompoundInitilizer;

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
    virtual void visit(ForInitExpression& node) = 0;
    virtual void visit(CastExpression& node) = 0;
    virtual void visit(AssignmentExpression& node) = 0;
    virtual void visit(ConditionalExpression& node) = 0;
    virtual void visit(FunctionCallExpression& node) = 0;
    virtual void visit(DereferenceExpression& node) = 0;
    virtual void visit(AddressOfExpression& node) = 0;
    virtual void visit(SubscriptExpression& node) = 0;
    virtual void visit(ExpressionStatement& node) = 0;
    virtual void visit(IfStatement& node) = 0;
    virtual void visit(NullStatement& node) = 0;
    virtual void visit(SingleInitializer& node) = 0;
    virtual void visit(CompoundInitilizer& node) = 0;
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

enum class StorageClass {
    NONE,
    STATIC,
    EXTERN
};

// Identifier remains unchanged as requested
class Identifier : public ParserAST {
public:
    Identifier(const std::string& name)
        : ParserAST(SourceLocationIndex(0)) // Placeholder since we're not tracking Identifier locations yet
        , name(name)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::string name;
};

class ForInit : public ParserAST {
public:
    virtual ~ForInit() = default;

protected:
    // Protected constructor for derived classes
    explicit ForInit(SourceLocationIndex loc)
        : ParserAST(loc)
    {
    }
};

// Abstract class for all expressions
class Expression : public ParserAST {
public:
    virtual ~Expression() = default;

    std::unique_ptr<Type> type { nullptr }; // this is set during type check phase

protected:
    // Protected constructor to be used by derived classes
    explicit Expression(SourceLocationIndex loc)
        : ParserAST(loc)
    {
    }
};

class ConstantExpression : public Expression {
public:
    template<typename T>
    ConstantExpression(SourceLocationIndex loc, T value)
        : Expression(loc)
        , value(value)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    ConstantType value;
};

class VariableExpression : public Expression {
public:
    VariableExpression(SourceLocationIndex loc, const std::string& id)
        : Expression(loc)
        , identifier { id }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
};

class CastExpression : public Expression {
public:
    CastExpression(SourceLocationIndex loc, std::unique_ptr<Type> target_type, std::unique_ptr<Expression> expression)
        : Expression(loc)
        , target_type(std::move(target_type))
        , expression(std::move(expression))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Type> target_type;
    std::unique_ptr<Expression> expression;
};

class UnaryExpression : public Expression {
public:
    UnaryExpression(SourceLocationIndex loc, UnaryOperator op, std::unique_ptr<Expression> expr)
        : Expression(loc)
        , unary_operator(op)
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
    BinaryExpression(SourceLocationIndex loc, BinaryOperator op, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : Expression(loc)
        , binary_operator(op)
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
    AssignmentExpression(SourceLocationIndex loc, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : Expression(loc)
        , left_expression(std::move(l))
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
    ConditionalExpression(SourceLocationIndex loc, std::unique_ptr<Expression> cond, std::unique_ptr<Expression> t, std::unique_ptr<Expression> f)
        : Expression(loc)
        , condition(std::move(cond))
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
    FunctionCallExpression(SourceLocationIndex loc, const std::string& n, std::vector<std::unique_ptr<Expression>> args)
        : Expression(loc)
        , name(n)
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

class DereferenceExpression : public Expression {
public:
    DereferenceExpression(SourceLocationIndex loc, std::unique_ptr<Expression> expr)
        : Expression(loc)
        , expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class AddressOfExpression : public Expression {
public:
    AddressOfExpression(SourceLocationIndex loc, std::unique_ptr<Expression> expr)
        : Expression(loc)
        , expression(std::move(expr))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class SubscriptExpression : public Expression {
public:
    SubscriptExpression(SourceLocationIndex loc, std::unique_ptr<Expression> expression1, std::unique_ptr<Expression> expression2)
        : Expression(loc)
        , expression1(std::move(expression1))
        , expression2(std::move(expression2))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression1;
    std::unique_ptr<Expression> expression2;

};

class BlockItem : public ParserAST {
public:
    virtual ~BlockItem() = default;

protected:
    // Protected constructor for derived classes
    explicit BlockItem(SourceLocationIndex loc)
        : ParserAST(loc)
    {
    }
};

class Block : public ParserAST {
public:
    Block(SourceLocationIndex loc, std::vector<std::unique_ptr<BlockItem>> i)
        : ParserAST(loc)
        , items(std::move(i))
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

protected:
    // Protected constructor for derived classes
    explicit Statement(SourceLocationIndex loc)
        : BlockItem(loc)
    {
    }
};

class ReturnStatement : public Statement {
public:
    ReturnStatement(SourceLocationIndex loc, std::unique_ptr<Expression> expr)
        : Statement(loc)
        , expression(std::move(expr))
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
    ExpressionStatement(SourceLocationIndex loc, std::unique_ptr<Expression> expr)
        : Statement(loc)
        , expression(std::move(expr))
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
    IfStatement(SourceLocationIndex loc, std::unique_ptr<Expression> cond, std::unique_ptr<Statement> then_stmt, std::unique_ptr<Statement> else_stmt = nullptr)
        : Statement(loc)
        , condition(std::move(cond))
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
    CompoundStatement(SourceLocationIndex loc, std::unique_ptr<Block> b)
        : Statement(loc)
        , block(std::move(b))
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
    BreakStatement(SourceLocationIndex loc, const std::string& l = "")
        : Statement(loc)
        , label { l }
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
    ContinueStatement(SourceLocationIndex loc, const std::string& l = "")
        : Statement(loc)
        , label { l }
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
    WhileStatement(SourceLocationIndex loc, std::unique_ptr<Expression> c, std::unique_ptr<Statement> b, const std::string& l = "")
        : Statement(loc)
        , condition { std::move(c) }
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
    DoWhileStatement(SourceLocationIndex loc, std::unique_ptr<Expression> c, std::unique_ptr<Statement> b, const std::string& l = "")
        : Statement(loc)
        , condition { std::move(c) }
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
    ForStatement(SourceLocationIndex loc, std::unique_ptr<ForInit> i, std::unique_ptr<Expression> c, std::unique_ptr<Expression> p, std::unique_ptr<Statement> b, const std::string& l = "")
        : Statement(loc)
        , init { std::move(i) }
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
    explicit NullStatement(SourceLocationIndex loc)
        : Statement(loc)
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }
};


class Initializer : public ParserAST{
public:
    explicit Initializer(SourceLocationIndex loc)
        : ParserAST(loc)
    {
    }
    virtual ~Initializer() = default;

    std::unique_ptr<Type> type { nullptr }; // this is set during type check phase
};

class SingleInitializer : public Initializer{
public:
    SingleInitializer(SourceLocationIndex loc, std::unique_ptr<Expression> expression)
        : Initializer(loc)
        , expression(std::move(expression)) {}

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::unique_ptr<Expression> expression;
};

class CompoundInitilizer : public Initializer{
public:
    CompoundInitilizer(SourceLocationIndex loc, std::vector<std::unique_ptr<Initializer>> initializer_list)
        : Initializer(loc)
        , initializer_list(std::move(initializer_list)) {}

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<Initializer>> initializer_list;
};


enum class DeclarationScope {
    File,
    Block
};

class Declaration : public BlockItem {
public:
    virtual ~Declaration() = default;

protected:
    // Protected constructor for derived classes
    explicit Declaration(SourceLocationIndex loc)
        : BlockItem(loc)
    {
    }
};



class VariableDeclaration : public Declaration {
public:
    VariableDeclaration(SourceLocationIndex loc, const std::string& identifier, std::unique_ptr<Initializer> expression, std::unique_ptr<Type> type, StorageClass storage_class, DeclarationScope scope)
        : Declaration(loc)
        , identifier { identifier }
        , expression(expression ? std::optional<std::unique_ptr<Initializer>>(std::move(expression)) : std::nullopt)
        , type { std::move(type) }
        , storage_class(storage_class)
        , scope { scope }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier identifier;
    std::optional<std::unique_ptr<Initializer>> expression;
    std::unique_ptr<Type> type;
    StorageClass storage_class;
    DeclarationScope scope;
};

class FunctionDeclaration : public Declaration {
public:
    FunctionDeclaration(SourceLocationIndex loc, const std::string& name, const std::vector<Identifier>& params, std::unique_ptr<Block> body, std::unique_ptr<Type> type,
        StorageClass storage_class, DeclarationScope scope)
        : Declaration(loc)
        , name(name)
        , params(params)
        , body(body != nullptr ? std::optional<std::unique_ptr<Block>>(std::move(body)) : std::nullopt)
        , type { std::move(type) }
        , storage_class(storage_class)
        , scope { scope }
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    Identifier name;
    std::vector<Identifier> params;
    std::optional<std::unique_ptr<Block>> body;
    std::unique_ptr<Type> type;
    StorageClass storage_class;
    DeclarationScope scope;
};

class ForInitDeclaration : public ForInit {
public:
    ForInitDeclaration(SourceLocationIndex loc, std::unique_ptr<VariableDeclaration> d)
        : ForInit(loc)
        , declaration { std::move(d) }
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
    ForInitExpression(SourceLocationIndex loc, std::unique_ptr<Expression> e)
        : ForInit(loc)
        , expression { e ? std::optional<std::unique_ptr<Expression>>(std::move(e)) : std::nullopt }
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
    Program(SourceLocationIndex loc, std::vector<std::unique_ptr<Declaration>> decls)
        : ParserAST(loc)
        , declarations(std::move(decls))
    {
    }

    void accept(ParserVisitor& visitor) override
    {
        visitor.visit(*this);
    }

    std::vector<std::unique_ptr<Declaration>> declarations;
};

}
