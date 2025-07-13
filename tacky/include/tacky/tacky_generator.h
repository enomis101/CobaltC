#pragma once
#include "common/data/name_generator.h"
#include "common/data/symbol_table.h"
#include "parser/parser_ast.h"
#include "tacky/tacky_ast.h"
#include <stdexcept>
#include <vector>

namespace tacky {

class TackyGeneratorError : public std::runtime_error {
public:
    explicit TackyGeneratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class ExpressionResult {
public:
    virtual ~ExpressionResult() = default;
};

class PlainOperand : public ExpressionResult {
public:
    PlainOperand(std::unique_ptr<Value> operand)
        : operand { std::move(operand) }
    {
    }

    std::unique_ptr<Value> operand;
};

class DereferencedPointer : public ExpressionResult {
public:
    DereferencedPointer(std::unique_ptr<Value> operand)
        : operand { std::move(operand) }
    {
    }

    std::unique_ptr<Value> operand;
};

// Generate a TackyAST from a ParserAST
class TackyGenerator {
public:
    TackyGenerator(std::shared_ptr<parser::ParserAST> ast, std::shared_ptr<NameGenerator> name_generator, std::shared_ptr<SymbolTable> symbol_table);

    std::shared_ptr<TackyAST> generate();

private:
    // Operator transformations
    UnaryOperator transform_unary_operator(parser::UnaryOperator& op);
    BinaryOperator transform_binary_operator(parser::BinaryOperator& op);

    // Main expression transformation dispatcher
    std::unique_ptr<ExpressionResult> emit_tacky(parser::Expression& expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_constant_expression(parser::ConstantExpression& constant_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_unary_expression(parser::UnaryExpression& unary_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_binary_expression(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_pointer_arithmetic_expression(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_logical_and(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_logical_or(parser::BinaryExpression& binary_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_variable_expression(parser::VariableExpression& variable_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_assignment_expression(parser::AssignmentExpression& assignment_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_conditional_expression(parser::ConditionalExpression& conditional_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_function_call_expression(parser::FunctionCallExpression& function_call_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_cast_expression(parser::CastExpression& cast_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_dereference_expression(parser::DereferenceExpression& dereference_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_address_of_expression(parser::AddressOfExpression& address_of_expression, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<ExpressionResult> transform_subscript_expression(parser::SubscriptExpression& subscript_expression, std::vector<std::unique_ptr<Instruction>>& instructions);

    std::unique_ptr<Value> emit_tacky_and_convert(parser::Expression& expr, std::vector<std::unique_ptr<Instruction>>& instructions);

    // Main statement transformation dispatcher
    void transform_statement(parser::Statement& statement, std::vector<std::unique_ptr<Instruction>>& instructions);

    // Specific statement transformations
    void transform_return_statement(parser::ReturnStatement& return_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_expression_statement(parser::ExpressionStatement& expression_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_if_statement(parser::IfStatement& if_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_compound_statement(parser::CompoundStatement& compound_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_break_statement(parser::BreakStatement& break_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_continue_statement(parser::ContinueStatement& continue_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_do_while_statement(parser::DoWhileStatement& do_while_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_while_statement(parser::WhileStatement& while_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_for_statement(parser::ForStatement& for_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_null_statement(parser::NullStatement& null_statement, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_compound_initializer(const parser::Identifier& identifier, const parser::Initializer& init, size_t& index, std::vector<std::unique_ptr<Instruction>>& instructions);

    // Other transformations
    void transform_declaration(parser::Declaration& declaration, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_for_init(parser::ForInit& for_init, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_block_item(parser::BlockItem& block_item, std::vector<std::unique_ptr<Instruction>>& instructions);
    void transform_block(parser::Block& block, std::vector<std::unique_ptr<Instruction>>& instructions);
    std::unique_ptr<FunctionDefinition> transform_function(parser::FunctionDeclaration& function);
    std::unique_ptr<TopLevel> transform_top_level_declaration(parser::Declaration& declaration);
    std::unique_ptr<Program> transform_program(parser::Program& program);

    void transform_symbols_to_tacky(std::shared_ptr<TackyAST> tacky_ast);
    size_t get_pointer_scale(const Type& type);

    // Create a new name for a temporary value and add it to the SymbolTable, we need to keep track of each TemporaryVariable type in the assembly stage
    // to determine operand size and stack space
    std::string make_and_add_temporary(const Type& type, const IdentifierAttribute& attr = LocalAttribute {});
    std::unique_ptr<TemporaryVariable> make_temporary_variable(const Type& type, const IdentifierAttribute& attr = LocalAttribute {});

    std::shared_ptr<parser::ParserAST> m_ast;
    std::shared_ptr<NameGenerator> m_name_generator;
    std::shared_ptr<SymbolTable> m_symbol_table;
};

} // namespace tacky
