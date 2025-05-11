#pragma once
#include "common/data/name_generator.h"
#include "parser/parser_ast.h"
#include "parser/semantic_analyzer_error.h"
#include <string>
#include <unordered_map>

namespace parser {

class VariableResolutionPass : public ParserVisitor {
public:
    VariableResolutionPass(std::shared_ptr<ParserAST> ast)
        : m_ast { ast }
        , m_name_generator { NameGenerator::instance() }
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
    void visit(FunctionCallExpression& node) override { }
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

    struct MapEntry {
        MapEntry() = default;
        MapEntry(const std::string& name, bool flag)
            : unique_name { name }
            , from_current_block { flag }
        {
        }
        std::string unique_name;
        bool from_current_block { false };
    };
    std::unordered_map<std::string, MapEntry> copy_variable_map();
    std::unordered_map<std::string, MapEntry> m_variable_map;
    std::shared_ptr<ParserAST> m_ast;
    NameGenerator& m_name_generator;
};

}
