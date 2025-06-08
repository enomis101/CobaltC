#pragma once
#include "common/data/source_location.h"
#include "common/data/source_manager.h"
#include "common/data/token.h"
#include "common/data/type.h"
#include "parser/parser_ast.h"
#include <memory>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace parser {

class Parser {
public:
    class ParserError : public std::runtime_error {
    public:
        explicit ParserError(const Parser& parser, const std::string& message)
            : std::runtime_error(message + parser.context_stack_to_string())
        {
        }
    };

    Parser(const std::vector<Token>& tokens, std::shared_ptr<SourceManager> source_manager)
        : m_tokens { tokens }
        , m_source_manager(source_manager)
    {
    }

    std::shared_ptr<Program> parse_program();
    std::string context_stack_to_string() const;

private:
    const std::vector<Token>& m_tokens;
    std::shared_ptr<SourceManager> m_source_manager;

    std::unique_ptr<Declaration> parse_declaration();
    std::unique_ptr<Block> parse_block();
    std::unique_ptr<BlockItem> parse_block_item();
    std::unique_ptr<ForInit> parse_for_init();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_conditional_middle();
    std::unique_ptr<Expression> parse_expression(int min_prec = 0);
    std::unique_ptr<Expression> parse_factor();
    std::unique_ptr<Type> parse_type();
    std::unique_ptr<Type> parse_type_specifier_list(const std::unordered_set<TokenType>& type_specifiers);
    void parse_parameter_list(std::vector<Identifier>& out_param_names, std::vector<std::unique_ptr<Type>>& out_param_types);

    std::pair<std::unique_ptr<Type>, StorageClass> parse_type_and_storage_class();

    UnaryOperator parse_unary_operator();
    BinaryOperator parse_binary_operator();
    StorageClass to_storage_class(TokenType tt);

    // Utility methods to check token types
    bool is_binary_operator(TokenType type);
    bool is_unary_operator(TokenType type);
    bool is_specificer(TokenType type);
    bool is_type_specificer(TokenType type);
    // std::unique_ptr<Identifier> parse_identifier();

    const Token& expect(TokenType expected);
    const Token& peek(int lh = 1);
    const Token& last_token();
    int precedence(const Token& token);
    void take_token();
    bool has_tokens();
    size_t i = 0;
    DeclarationScope m_current_declaration_scope;

    using ContextStack = std::vector<std::string>;

    ContextStack m_context_stack;

    class ContextGuard {
    public:
        ContextGuard(ContextStack& context_stack, const std::string& context, std::optional<SourceLocation> source_location);

        ~ContextGuard();

    private:
        ContextStack& m_context_stack;
    };
};
}
