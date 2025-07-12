#pragma once
#include "common/data/name_generator.h"
#include "parser/parser_ast.h"
#include "parser/semantic_analyzer_error.h"
#include <stack>
#include <string>

namespace parser {

class LoopLabelingPassError : public SemanticAnalyzerError {
public:
    explicit LoopLabelingPassError(const std::string& message)
        : SemanticAnalyzerError(message)
    {
    }
};

class LoopLabelingPass : public ParserVisitor {
public:
    LoopLabelingPass(std::shared_ptr<ParserAST> ast, std::shared_ptr<NameGenerator> name_generator)
        : m_ast { ast }
        , m_name_generator { name_generator }
    {
    }
    void run();

private:
    void visit(Identifier& node) override;
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(Program& node) override;
    void visit(VariableExpression& node) override;
    void visit(CastExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(FunctionCallExpression& node) override { }
    void visit(DereferenceExpression& node) override { }
    void visit(AddressOfExpression& node) override { }
    void visit(SubscriptExpression& node) override { } // TODO: IMPLEMENT
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(SingleInitializer& node) override { }   // TODO: IMPLEMENT
    void visit(CompoundInitializer& node) override { } // TODO: IMPLEMENT
    void visit(VariableDeclaration& node) override;
    void visit(Block& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(ContinueStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ForInitDeclaration& node) override;
    void visit(ForInitExpression& node) override;

    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<NameGenerator> m_name_generator;
    std::stack<std::string> m_label_stack;
};

}
