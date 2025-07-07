#pragma once
#include "common/data/source_manager.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "common/data/warning_manager.h"
#include "parser/parser_ast.h"
#include "parser/semantic_analyzer_error.h"
#include <memory>
#include <string>

namespace parser {

class TypeCheckPassError : public SemanticAnalyzerError {
public:
    explicit TypeCheckPassError(const std::string& message)
        : SemanticAnalyzerError("TypeCheckPassError: " + message)
    {
    }
};

class TypeCheckPass : public ParserVisitor {
public:
    TypeCheckPass(std::shared_ptr<ParserAST> ast, std::shared_ptr<SymbolTable> symbol_table, std::shared_ptr<SourceManager> source_manager, std::shared_ptr<WarningManager> warning_manager)
        : m_ast { ast }
        , m_symbol_table { symbol_table }
        , m_source_manager(source_manager)
        , m_warning_manager { warning_manager }
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
    void visit(FunctionCallExpression& node) override;
    void visit(DereferenceExpression& node) override;
    void visit(AddressOfExpression& node) override;
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

    std::unique_ptr<Type> get_common_type(const Type& t1, const Type& t2);
    std::optional<std::unique_ptr<Type>> get_common_pointer_type(const Expression& expr1, const Expression& expr2);
    bool is_null_pointer_constant_expression(const Expression& expr);
    bool is_null_pointer_constant(const ConstantType& constant_type);

    bool convert_expression_by_assignment(std::unique_ptr<Expression>& expr, const Type& target_type);
    void convert_expression_to(std::unique_ptr<Expression>& expr, const Type& target_type);
    std::expected<StaticInitialValueType, std::string> convert_constant_type_by_assignment(const ConstantType& value, const Type& target_type, std::function<void(const std::string&)> warning_callback = nullptr);

    bool is_lvalue(const Expression& expr);

    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
    std::shared_ptr<SourceManager> m_source_manager;
    std::shared_ptr<WarningManager> m_warning_manager;
    FunctionDeclaration* m_current_function_declaration; // needed to map a return statement to a function delcaration
};

}
