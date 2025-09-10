#pragma once
#include "common/data/source_manager.h"
#include "common/data/symbol_table.h"
#include "common/data/type.h"
#include "common/data/warning_manager.h"
#include "common/error/internal_compiler_error.h"
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
    // Core expression type checking functions
    void typecheck_expression_and_convert(std::unique_ptr<Expression>& expr);
    void typecheck_expression(Expression& expr);

    // Individual expression type checking methods
    void typecheck_constant_expression(ConstantExpression& node);
    void typecheck_variable_expression(VariableExpression& node);
    void typecheck_unary_expression(UnaryExpression& node);
    void typecheck_binary_expression(BinaryExpression& node);
    void typecheck_assignment_expression(AssignmentExpression& node);
    void typecheck_conditional_expression(ConditionalExpression& node);
    void typecheck_function_call_expression(FunctionCallExpression& node);
    void typecheck_cast_expression(CastExpression& node);
    void typecheck_dereference_expression(DereferenceExpression& node);
    void typecheck_address_of_expression(AddressOfExpression& node);
    void typecheck_subscript_expression(SubscriptExpression& node);

    void typecheck_initializer(const Type& target_type, Initializer& init);
    std::unique_ptr<Initializer> get_zero_initializer(SourceLocationIndex loc, const Type& type);

    StaticInitialValue convert_static_initializer(const Type& target_type, Initializer& init, std::function<void(const std::string&)> warning_callback);
    size_t get_static_zero_initializer(const Type& target_type);

    // Visitor methods for non-expression nodes
    void visit(Identifier& node) override;
    void visit(ReturnStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(Program& node) override;
    void visit(ExpressionStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(NullStatement& node) override;
    void visit(SingleInitializer& node) override { throw InternalCompilerError("visit(Initializer&) should not be called - use typecheck_initializer instead"); }
    void visit(CompoundInitializer& node) override { throw InternalCompilerError("visit(Initializer&) should not be called - use typecheck_initializer instead"); }
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

    // Expression visitor methods - kept for ParserVisitor interface compatibility but will throw errors
    void visit(UnaryExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(BinaryExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(ConstantExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(VariableExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(CastExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(AssignmentExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(ConditionalExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(FunctionCallExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(DereferenceExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(AddressOfExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(SubscriptExpression& node) override { throw InternalCompilerError("visit(Expression&) should not be called - use typecheck_expression_and_convert instead"); }
    void visit(StringExpression& node) override { } // TODO: IMPLEMENT

    void typecheck_file_scope_variable_declaration(VariableDeclaration& variable_declaration);
    void typecheck_local_variable_declaration(VariableDeclaration& variable_declaration);

    std::unique_ptr<Type> get_common_type(const Type& t1, const Type& t2);
    std::unique_ptr<Type> get_common_type(const Expression& expr1, const Expression& expr2);
    std::unique_ptr<Type> get_common_pointer_type(const Expression& expr1, const Expression& expr2);
    bool is_null_pointer_constant_expression(const Expression& expr);

    bool convert_expression_by_assignment(std::unique_ptr<Expression>& expr, const Type& target_type);

    template<typename T>
    requires std::derived_from<T, Type>
    void convert_expression_to(std::unique_ptr<Expression>& expr)
    {
        auto type = std::make_unique<T>();
        return convert_expression_to(expr, *type);
    }

    void convert_expression_to(std::unique_ptr<Expression>& expr, const Type& target_type);

    StaticInitialValue convert_constant_type_by_assignment(const ConstantType& value, const Type& target_type, SourceLocationIndex loc, std::function<void(const std::string&)> warning_callback = nullptr);

    bool is_lvalue(const Expression& expr);

    std::shared_ptr<ParserAST> m_ast;
    std::shared_ptr<SymbolTable> m_symbol_table;
    std::shared_ptr<SourceManager> m_source_manager;
    std::shared_ptr<WarningManager> m_warning_manager;
    FunctionDeclaration* m_current_function_declaration; // needed to map a return statement to a function delcaration
};

}
