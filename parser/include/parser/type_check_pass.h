#pragma once
#include "common/data/symbol_table.h"
#include "parser/parser_ast.h"
#include "parser/semantic_analyzer_error.h"
#include <string>

namespace parser {

class TypeCheckPassError : public SemanticAnalyzerError {
public:
    explicit TypeCheckPassError(const std::string& message)
        : SemanticAnalyzerError(message)
    {
    }
};

class TypeCheckPass : public ParserVisitor {
public:
    TypeCheckPass(std::shared_ptr<ParserAST> ast, std::shared_ptr<SymbolTable> symbol_table)
        : m_ast { ast }
        , m_symbol_table { symbol_table }
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
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(FunctionCallExpression& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(Block& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BreakStatement& node) override { }
    void visit(ContinueStatement& node) override { }
    void visit(WhileStatement& node) override;
    void visit(DoWhileStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ForInitDeclaration& node) override;
    void visit(ForInitExpression& node) override;

    void resolve_function_param_declaration(Identifier& identifier);

    void typecheck_file_scope_variable_declaration(VariableDeclaration& variable_declaration);
    void typecheck_local_variable_declaration(VariableDeclaration& variable_declaration);

    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
};

}
