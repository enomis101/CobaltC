#pragma once
#include "common/data/name_generator.h"
#include "parser/parser_ast.h"
#include <string>
#include <unordered_map>
#include "parser/semantic_analyzer_error.h"

namespace parser {

class IdentifierResolutionPassError : public SemanticAnalyzerError {
public:
    explicit IdentifierResolutionPassError(const std::string& message)
        : SemanticAnalyzerError(message)
    {
    }
};

class IdentifierResolutionPass : public ParserVisitor {
public:
    IdentifierResolutionPass(std::shared_ptr<ParserAST> ast)
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

    struct MapEntry {
        MapEntry() = default;
        MapEntry(const std::string& name, bool flag, bool link)
            : new_name { name }
            , from_current_scope { flag }
            , has_linkage { link }
        {
        }
        std::string new_name;
        bool from_current_scope { false };
        bool has_linkage { false };
    };
    using IdentifierMap = std::unordered_map<std::string, MapEntry>;

    class IdentifierMapGuard {
    public:
        IdentifierMapGuard(IdentifierMap& id_map)
            : m_identifier_map { id_map }
            , m_old_map { id_map }
        {
            for (auto& p : m_identifier_map) {
                p.second.from_current_scope = false;
            }
        }

        ~IdentifierMapGuard()
        {
            m_identifier_map = m_old_map;
        }

        IdentifierMap& m_identifier_map;
        IdentifierMap m_old_map;
    };

    void resolve_variable_declaration(Identifier& identifier);

    bool is_top_level(FunctionDeclaration& fun_decl);

    IdentifierMap m_identifier_map;
    std::shared_ptr<ParserAST> m_ast;
    NameGenerator& m_name_generator;
    std::unordered_map<FunctionDeclaration*, bool> m_top_level_tracker;
};

}
