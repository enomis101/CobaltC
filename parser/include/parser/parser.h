#pragma once
#include "common/data/source_location.h"
#include "common/data/source_manager.h"
#include "common/data/token.h"
#include "common/data/type.h"
#include "parser/parser_ast.h"
#include <memory>
#include <stdexcept>
#include <vector>

namespace parser {

class DeclaratorError : public std::runtime_error {
public:
    explicit DeclaratorError(const std::string& message)
        : std::runtime_error(message)
    {
    }
};

class AbstractDeclarator {
public:
    virtual ~AbstractDeclarator() = default;
};

class BaseAbstractDeclarator : public AbstractDeclarator {
public:
};

class PointerAbstractDeclarator : public AbstractDeclarator {
public:
    PointerAbstractDeclarator(std::unique_ptr<AbstractDeclarator> declarator)
        : declarator(std::move(declarator))
    {
    }
    std::unique_ptr<AbstractDeclarator> declarator;
};

class ArrayAbstractDeclarator : public AbstractDeclarator {
public:
    ArrayAbstractDeclarator(std::unique_ptr<AbstractDeclarator> element_declarator, size_t size)
        : element_declarator(std::move(element_declarator))
        , size{size}
    {
    }
    std::unique_ptr<AbstractDeclarator> element_declarator;
    size_t size;
};

class Declarator {
public:
    virtual ~Declarator() = default;
};

class ParameterDeclaratorInfo {
public:
    ParameterDeclaratorInfo(std::unique_ptr<Type> parameter_type, std::unique_ptr<Declarator> parameter_declarator)
        : parameter_type(std::move(parameter_type))
        , parameter_declarator(std::move(parameter_declarator))
    {
    }
    std::unique_ptr<Type> parameter_type;
    std::unique_ptr<Declarator> parameter_declarator;
};

class IdentifierDeclarator : public Declarator {
public:
    IdentifierDeclarator(const std::string& identifier)
        : identifier(identifier)
    {
    }

    std::string identifier;
};

class PointerDeclarator : public Declarator {
public:
    PointerDeclarator(std::unique_ptr<Declarator> inner_declarator)
        : inner_declarator(std::move(inner_declarator))
    {
    }
    std::unique_ptr<Declarator> inner_declarator;
};

class ArrayDeclarator : public Declarator {
public:
    ArrayDeclarator(std::unique_ptr<Declarator> element_declarator, size_t size)
        : element_declarator(std::move(element_declarator))
        , size{size}
    {
    }
    std::unique_ptr<Declarator> element_declarator;
    size_t size;
};

class FunctionDeclarator : public Declarator {
public:
    FunctionDeclarator(std::vector<ParameterDeclaratorInfo>&& parameters, std::unique_ptr<Declarator> declarator)
        : parameters(std::move(parameters))
        , declarator(std::move(declarator))
    {
    }
    std::vector<ParameterDeclaratorInfo> parameters;
    std::unique_ptr<Declarator> declarator;
};

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
    std::unique_ptr<Declarator> parse_declarator();
    std::unique_ptr<Declarator> parse_direct_declarator();
    std::unique_ptr<Declarator> parse_simple_declarator();

    std::unique_ptr<AbstractDeclarator> parse_abstract_declarator();
    std::unique_ptr<AbstractDeclarator> parse_direct_abstract_declarator();

    std::unique_ptr<Block> parse_block();
    std::unique_ptr<BlockItem> parse_block_item();
    std::unique_ptr<ForInit> parse_for_init();
    std::unique_ptr<Statement> parse_statement();
    std::unique_ptr<Expression> parse_conditional_middle();
    std::unique_ptr<Expression> parse_expression(int min_prec = 0);
    std::unique_ptr<Expression> parse_factor();
    std::unique_ptr<Type> parse_type();
    std::unique_ptr<Type> parse_type_specifier_list(const std::vector<TokenType>& type_specifiers);
    std::unique_ptr<Initializer> parse_initializer();
    void parse_parameter_list(std::vector<ParameterDeclaratorInfo>& out_params);

    std::pair<std::unique_ptr<Type>, StorageClass> parse_type_and_storage_class();


    UnaryOperator parse_unary_operator();
    BinaryOperator parse_binary_operator();
    std::unique_ptr<Expression> parse_contant();

    StorageClass to_storage_class(TokenType tt);

    std::tuple<std::string, std::unique_ptr<Type>, std::vector<Identifier>> process_declarator(const Declarator& declarator, const Type& type);
    std::unique_ptr<Type> process_abstract_declarator(const AbstractDeclarator& declarator, const Type& base_type);

    // Utility methods to check token types
    bool is_binary_operator(TokenType type);
    bool is_unary_operator(TokenType type);
    bool is_specificer(TokenType type);
    bool is_type_specificer(TokenType type);
    bool is_constant(TokenType type);
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
