#pragma once
#include "common/data/name_generator.h"
#include "parser/parser_ast.h"
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace parser {

class SemanticAnalyzerError : public std::runtime_error {
public:
    explicit SemanticAnalyzerError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class SemanticAnalyzer : public ParserVisitor {
public:
    SemanticAnalyzer(std::shared_ptr<ParserAST> ast)
        : m_ast { ast }
        , m_name_generator { NameGenerator::instance() }
    {
    }
    void analyze();

private:
    void visit(Identifier& node) override;
    void visit(UnaryExpression& node) override;
    void visit(BinaryExpression& node) override;
    void visit(ConstantExpression& node) override;
    void visit(ReturnStatement& node) override;
    void visit(Function& node) override;
    void visit(Program& node) override;
    void visit(VariableExpression& node) override;
    void visit(AssignmentExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(VariableDeclaration& node) override;
    void visit(Block& node) override {}
    void visit(CompoundStatement& node) override{}

    std::unordered_map<std::string, std::string> m_variable_map;
    std::shared_ptr<ParserAST> m_ast;
    NameGenerator& m_name_generator;
};

}
