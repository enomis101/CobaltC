#include "parser/parser.h"
#include "common/data/source_location.h"

#include "common/data/token.h"
#include "common/data/token_table.h"
#include "common/data/type.h"
#include "parser/parser_ast.h"
#include <cassert>
#include <format>
#include <memory>
#include <unordered_set>
#include <vector>

using namespace parser;

#define ENTER_CONTEXT(name) ContextGuard context_guard(m_context_stack, name, (has_tokens() ? std::optional<SourceLocation>(peek().source_location()) : std::nullopt))

std::shared_ptr<Program> Parser::parse_program()
{
    ENTER_CONTEXT("parse_program");

    SourceLocationIndex loc = m_source_manager->get_index(m_tokens.at(0));

    std::vector<std::unique_ptr<Declaration>> decls;
    while (has_tokens()) {
        m_current_declaration_scope = DeclarationScope::File;
        decls.emplace_back(parse_declaration());
    }
    return std::make_shared<Program>(loc, std::move(decls));
}

std::unique_ptr<Block> Parser::parse_block()
{
    ENTER_CONTEXT("parse_block");

    m_current_declaration_scope = DeclarationScope::Block;
    const Token& brace_token = expect(TokenType::OPEN_BRACE);
    const Token* next_token = has_tokens() ? &peek() : nullptr;
    std::vector<std::unique_ptr<BlockItem>> body;
    while (next_token && next_token->type() != TokenType::CLOSE_BRACE) {
        body.emplace_back(parse_block_item());
        next_token = has_tokens() ? &peek() : nullptr;
    }
    expect(TokenType::CLOSE_BRACE);
    SourceLocationIndex loc = m_source_manager->get_index(brace_token);
    return std::make_unique<Block>(loc, std::move(body));
}

std::unique_ptr<BlockItem> Parser::parse_block_item()
{
    ENTER_CONTEXT("parse_block_item");

    const Token& next_token = peek();
    if (is_specificer(next_token.type())) {
        return parse_declaration();
    } else {
        return parse_statement();
    }
}

void Parser::parse_parameter_list(std::vector<ParameterDeclaratorInfo>& out_params)
{
    ENTER_CONTEXT("parse_parameter_list");

    expect(TokenType::OPEN_PAREN);

    const Token* next_token = &peek();
    if (next_token->type() == TokenType::VOID_KW) {
        expect(TokenType::VOID_KW);
        return;
    }

    while (true) {
        auto type = parse_type();
        auto declarator = parse_declarator();
        out_params.emplace_back(std::move(type), std::move(declarator));
        next_token = &peek();
        if (next_token->type() == TokenType::CLOSE_PAREN) {
            break;
        }
        expect(TokenType::COMMA);
    }

    expect(TokenType::CLOSE_PAREN);
}

std::unique_ptr<Declaration> Parser::parse_declaration()
{
    ENTER_CONTEXT("parse_declaration");

    SourceLocationIndex start_loc = m_source_manager->get_index(peek());

    DeclarationScope current_declaration_scope = m_current_declaration_scope;
    auto [base_type, storage_class] = parse_type_and_storage_class();
    auto declarator = parse_declarator();
    auto [name, derived_type, param_names] = process_declarator(*declarator, *base_type);

    if (is_type<FunctionType>(*derived_type)) {
        auto next_token = peek();
        std::unique_ptr<Block> body = nullptr;
        if (next_token.type() == TokenType::SEMICOLON) {
            expect(TokenType::SEMICOLON);
        } else {
            body = parse_block();
        }

        return std::make_unique<FunctionDeclaration>(start_loc, name, param_names, std::move(body), std::move(derived_type), storage_class, current_declaration_scope);
    } else {
        auto next_token = peek();
        std::unique_ptr<Expression> init_expr = nullptr;
        if (next_token.type() != TokenType::SEMICOLON) {
            expect(TokenType::ASSIGNMENT);
            init_expr = parse_expression();
            expect(TokenType::SEMICOLON);
        } else {
            expect(TokenType::SEMICOLON);
        }

        return std::make_unique<VariableDeclaration>(start_loc, name, std::move(init_expr), std::move(derived_type), storage_class, current_declaration_scope);
    }
}

std::unique_ptr<Declarator> Parser::parse_declarator()
{
    ENTER_CONTEXT("parse_declarator");

    const Token& next_token = peek();
    if(next_token.type() == TokenType::ASTERISK){
        expect(TokenType::ASTERISK);
        auto decl = parse_declarator();
        return std::make_unique<PointerDeclarator>(std::move(decl));
    } else{
        return parse_direct_declarator();
    }
}

std::unique_ptr<Declarator> Parser::parse_direct_declarator()
{
    ENTER_CONTEXT("parse_simple_declarator");

    auto simple_declarator = parse_simple_declarator();
    const Token& next_token = peek();
    if(next_token.type() == TokenType::OPEN_PAREN){
        std::vector<ParameterDeclaratorInfo> params;
        parse_parameter_list(params);
        return std::make_unique<FunctionDeclarator>(std::move(params), std::move(simple_declarator));
    } else {
      return simple_declarator;
    }
}

std::unique_ptr<Declarator> Parser::parse_simple_declarator()
{
    ENTER_CONTEXT("parse_simple_declarator");

    const Token& next_token = peek();
    SourceLocationIndex loc = m_source_manager->get_index(next_token);
    if(next_token.type() == TokenType::IDENTIFIER){
        const Token& identifier_token = expect(TokenType::IDENTIFIER);
        return std::make_unique<IdentifierDeclarator>(identifier_token.lexeme());
    } else if(next_token.type() == TokenType::OPEN_PAREN){
        expect(TokenType::OPEN_PAREN);
        auto decl = parse_declarator();
        expect(TokenType::CLOSE_PAREN);
        return decl;
    } else{
        throw ParserError(*this, std::format("Error in parse_simple_declarator at\n{}", m_source_manager->get_source_line(loc)));
    }
}

std::unique_ptr<ForInit> Parser::parse_for_init()
{
    ENTER_CONTEXT("parse_for_init");

    const Token& next_token = peek();
    SourceLocationIndex loc = m_source_manager->get_index(next_token);
    if (is_specificer(next_token.type())) {
        std::unique_ptr<Declaration> decl = parse_declaration();

        if (!dynamic_cast<VariableDeclaration*>(decl.get())) {
            throw ParserError(*this,
                std::format("In parse_for_init: got FunctionDeclaration, expected VariableDeclaration at:\n{}", m_source_manager->get_source_line(peek().source_location())));
        }

        // Release from original unique_ptr and wrap in new one
        std::unique_ptr<VariableDeclaration> var_decl(dynamic_cast<VariableDeclaration*>(decl.release()));
        return std::make_unique<ForInitDeclaration>(loc, std::move(var_decl));
    } else {
        std::unique_ptr<Expression> e = (next_token.type() == TokenType::SEMICOLON) ? nullptr : parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ForInitExpression>(loc, std::move(e));
    }
}

std::unique_ptr<Statement> Parser::parse_statement()
{
    ENTER_CONTEXT("parse_statement");

    const Token& next_token = peek();
    SourceLocationIndex loc = m_source_manager->get_index(next_token);
    switch (next_token.type()) {
    case TokenType::RETURN_KW: {
        expect(TokenType::RETURN_KW);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ReturnStatement>(loc, std::move(expr));
    }
    case TokenType::IF_KW: {
        expect(TokenType::IF_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        std::unique_ptr<Statement> then_statement = parse_statement();
        std::unique_ptr<Statement> else_statement = nullptr;
        const Token& if_next_token = peek();
        if (if_next_token.type() == TokenType::ELSE_KW) {
            take_token();
            else_statement = parse_statement();
        }
        return std::make_unique<IfStatement>(loc, std::move(expr), std::move(then_statement), std::move(else_statement));
    }
    case TokenType::OPEN_BRACE: {
        std::unique_ptr<Block> block = parse_block();
        return std::make_unique<CompoundStatement>(loc, std::move(block));
    }
    case TokenType::SEMICOLON: {
        expect(TokenType::SEMICOLON);
        return std::make_unique<NullStatement>(loc);
    }
    case TokenType::BREAK_KW: {
        expect(TokenType::BREAK_KW);
        expect(TokenType::SEMICOLON);
        return std::make_unique<BreakStatement>(loc);
    }
    case TokenType::CONTINUE_KW: {
        expect(TokenType::CONTINUE_KW);
        expect(TokenType::SEMICOLON);
        return std::make_unique<ContinueStatement>(loc);
    }
    case TokenType::WHILE_KW: {
        expect(TokenType::WHILE_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        std::unique_ptr<Statement> statement = parse_statement();
        return std::make_unique<WhileStatement>(loc, std::move(expr), std::move(statement));
    }
    case TokenType::DO_KW: {
        expect(TokenType::DO_KW);
        std::unique_ptr<Statement> statement = parse_statement();
        expect(TokenType::WHILE_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        expect(TokenType::SEMICOLON);
        return std::make_unique<DoWhileStatement>(loc, std::move(expr), std::move(statement));
    }
    case TokenType::FOR_KW: {
        expect(TokenType::FOR_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<ForInit> for_init = parse_for_init();
        std::unique_ptr<Expression> cond = nullptr;
        {
            const Token& for_next_token = peek();
            if (for_next_token.type() != TokenType::SEMICOLON) {
                cond = parse_expression();
            }
        }
        expect(TokenType::SEMICOLON);
        std::unique_ptr<Expression> post = nullptr;
        {
            const Token& for_next_token = peek();
            if (for_next_token.type() != TokenType::CLOSE_PAREN) {
                post = parse_expression();
            }
        }
        expect(TokenType::CLOSE_PAREN);
        std::unique_ptr<Statement> statement = parse_statement();
        return std::make_unique<ForStatement>(loc, std::move(for_init), std::move(cond), std::move(post), std::move(statement));
    }
    default: {
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ExpressionStatement>(loc, std::move(expr));
    }
    }
}

std::unique_ptr<Expression> Parser::parse_conditional_middle()
{
    ENTER_CONTEXT("parse_conditional_middle");

    expect(TokenType::QUESTION_MARK);
    std::unique_ptr<Expression> expr = parse_expression(0); // reset back to zero precedence level
    expect(TokenType::COLON);
    return expr;
}

// Implement precedence climbing
std::unique_ptr<Expression> Parser::parse_expression(int min_prec)
{
    ENTER_CONTEXT("parse_expression");

    std::unique_ptr<Expression> left;

    left = parse_factor();
    if (!has_tokens()) {
        return left;
    }

    const Token* next_token = &peek();
    while (next_token && is_binary_operator(next_token->type()) && precedence(*next_token) >= min_prec) {
        SourceLocationIndex loc = m_source_manager->get_index(*next_token);
        if (next_token->type() == TokenType::ASSIGNMENT) { //= must be rigth associative a = b = c --> a  = (b = c)
            take_token();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token)); // different than binary operator because it's rigth associative
            left = std::make_unique<AssignmentExpression>(loc, std::move(left), std::move(right));
        } else if (next_token->type() == TokenType::QUESTION_MARK) {
            // QUESTION_MARK consumed by parse_conditional_middle
            std::unique_ptr<Expression> middle = parse_conditional_middle();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token)); // different than binary operator because it's rigth associative
            left = std::make_unique<ConditionalExpression>(loc, std::move(left), std::move(middle), std::move(right));
        } else {
            BinaryOperator op = parse_binary_operator();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token) + 1);
            left = std::make_unique<BinaryExpression>(loc, op, std::move(left), std::move(right));
        }

        next_token = has_tokens() ? &peek() : nullptr;
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_factor()
{
    ENTER_CONTEXT("parse_factor");

    const Token& next_token = peek();
    SourceLocationIndex loc = m_source_manager->get_index(next_token);
    if (is_constant(next_token.type())) {
        return parse_contant();
    } else if (is_unary_operator(next_token.type())) {
        UnaryOperator op = parse_unary_operator();
        std::unique_ptr<Expression> expr = parse_factor();
        return std::make_unique<UnaryExpression>(loc, op, std::move(expr));
    } else if (next_token.type() == TokenType::OPEN_PAREN) {
        expect(TokenType::OPEN_PAREN);
        const Token* new_next_token = &peek();
        if (is_type_specificer(new_next_token->type())) { // CAST
            std::unique_ptr<Type> type = parse_type();
            expect(TokenType::CLOSE_PAREN);
            std::unique_ptr<Expression> factor = parse_factor();
            return std::make_unique<CastExpression>(loc, std::move(type), std::move(factor));
        } else {
            std::unique_ptr<Expression> res = parse_expression();
            expect(TokenType::CLOSE_PAREN);
            return res;
        }
    } else if (next_token.type() == TokenType::IDENTIFIER) {
        const Token& identifier_token = expect(TokenType::IDENTIFIER);
        const Token* new_next_token = &peek();
        if (new_next_token->type() != TokenType::OPEN_PAREN) {
            return std::make_unique<VariableExpression>(loc, identifier_token.lexeme());
        }
        expect(TokenType::OPEN_PAREN);
        std::vector<std::unique_ptr<Expression>> args;
        new_next_token = &peek();
        if (new_next_token->type() != TokenType::CLOSE_PAREN) {
            while (true) {
                args.emplace_back(parse_expression());
                new_next_token = &peek();
                if (new_next_token->type() == TokenType::CLOSE_PAREN) {
                    break;
                }
                expect(TokenType::COMMA);
            }
        }
        expect(TokenType::CLOSE_PAREN);
        return std::make_unique<FunctionCallExpression>(loc, identifier_token.lexeme(), std::move(args));
    } else {
        throw ParserError(*this, std::format("Malformed Factor at\n{}", m_source_manager->get_source_line(next_token.source_location())));
    }
}

std::unique_ptr<Type> Parser::parse_type()
{
    ENTER_CONTEXT("parse_type");

    std::vector<TokenType> specifiers;
    const Token* curr_token = &peek();
    while (is_type_specificer(curr_token->type())) {
        take_token();
        TokenType tt = curr_token->type();
        specifiers.push_back(tt);

        curr_token = &peek();
    }
    return parse_type_specifier_list(specifiers);
}

std::unique_ptr<Type> Parser::parse_type_specifier_list(const std::vector<TokenType>& type_specifiers)
{
    ENTER_CONTEXT("parse_type_specifier_list");

    std::unordered_set<TokenType> type_specifiers_set;
    for (auto tt : type_specifiers) {
        auto res = type_specifiers_set.insert(tt);

        if (!res.second) {
            throw ParserError(*this, std::format("Multiple Type Specifier {} at\n{}", Token::type_to_string(tt), m_source_manager->get_source_line(last_token().source_location())));
        }

        if (!is_type_specificer(tt)) {
            throw ParserError(*this, std::format("Type specifier contains invalid type:\n{}", m_source_manager->get_source_line(last_token().source_location())));
        }
    }

    if (type_specifiers_set.empty()) {
        throw ParserError(*this, std::format("Missing type at:\n{}", m_source_manager->get_source_line(last_token().source_location())));
    }

    if (type_specifiers_set.contains(TokenType::SIGNED_KW) && type_specifiers_set.contains(TokenType::UNSIGNED_KW)) {
        throw ParserError(*this, std::format("Type specifier with both signed and unsigned at:\n{}", m_source_manager->get_source_line(last_token().source_location())));
    }

    if (type_specifiers_set.size() == 1 && type_specifiers_set.contains(TokenType::DOUBLE_KW)) {
        return std::make_unique<DoubleType>();
    } else if (type_specifiers_set.contains(TokenType::DOUBLE_KW)) {
        throw ParserError(*this, std::format("Can't combine double with other type specifiers at:\n{}", m_source_manager->get_source_line(last_token().source_location())));
    }

    if (type_specifiers_set.contains(TokenType::LONG_KW) && type_specifiers_set.contains(TokenType::UNSIGNED_KW)) {
        return std::make_unique<UnsignedLongType>();
    } else if (type_specifiers_set.contains(TokenType::UNSIGNED_KW)) {
        return std::make_unique<UnsignedIntType>();
    } else if (type_specifiers_set.contains(TokenType::LONG_KW)) {
        return std::make_unique<LongType>();
    } else {
        return std::make_unique<IntType>();
    }
}

UnaryOperator Parser::parse_unary_operator()
{
    ENTER_CONTEXT("parse_unary_operator");
    static const std::unordered_map<TokenType, UnaryOperator> unary_op_map = {
        { TokenType::MINUS, UnaryOperator::NEGATE },
        { TokenType::COMPLEMENT, UnaryOperator::COMPLEMENT },
        { TokenType::EXCLAMATION_POINT, UnaryOperator::NOT }
    };

    const Token& next_token = peek();
    auto it = unary_op_map.find(next_token.type());
    if (it == unary_op_map.end()) {
        throw ParserError(*this, std::format("Unsupported Unary Operator {}", Token::type_to_string(next_token.type())));
    }

    take_token();
    return it->second;
}

BinaryOperator Parser::parse_binary_operator()
{
    ENTER_CONTEXT("parse_binary_operator");

    static const std::unordered_map<TokenType, BinaryOperator> binary_op_map = {
        { TokenType::ASTERISK, BinaryOperator::MULTIPLY },
        { TokenType::FORWARD_SLASH, BinaryOperator::DIVIDE },
        { TokenType::PERCENT, BinaryOperator::REMAINDER },
        { TokenType::PLUS, BinaryOperator::ADD },
        { TokenType::MINUS, BinaryOperator::SUBTRACT },
        { TokenType::LOGICAL_AND, BinaryOperator::AND },
        { TokenType::LOGICAL_OR, BinaryOperator::OR },
        { TokenType::EQUAL, BinaryOperator::EQUAL },
        { TokenType::NOT_EQUAL, BinaryOperator::NOT_EQUAL },
        { TokenType::LESS_THAN, BinaryOperator::LESS_THAN },
        { TokenType::LESS_THAN_EQUAL, BinaryOperator::LESS_OR_EQUAL },
        { TokenType::GREATER_THAN, BinaryOperator::GREATER_THAN },
        { TokenType::GREATER_THAN_EQUAL, BinaryOperator::GREATER_OR_EQUAL }
    };

    const Token& next_token = peek();

    auto it = binary_op_map.find(next_token.type());

    if (it == binary_op_map.end()) {
        throw ParserError(*this, std::format("Unsupported Binary Operator {}", Token::type_to_string(next_token.type())));
    }

    take_token();
    return it->second;
}

std::unique_ptr<Expression> Parser::parse_contant()
{
    ENTER_CONTEXT("parse_contant");
    const Token& next_token = peek();
    SourceLocationIndex loc = m_source_manager->get_index(next_token);
    if (!is_constant(next_token.type())) {
        throw ParserError(*this, std::format("parse_contant called with non constant token {}", Token::type_to_string(next_token.type())));
    }
    take_token();
    if (next_token.type() == TokenType::CONSTANT) {
        return std::make_unique<ConstantExpression>(loc, next_token.literal<int>());
    } else if (next_token.type() == TokenType::UNSIGNED_CONSTANT) {
        return std::make_unique<ConstantExpression>(loc, next_token.literal<unsigned int>());
    } else if (next_token.type() == TokenType::LONG_CONSTANT) {
        return std::make_unique<ConstantExpression>(loc, next_token.literal<long>());
    } else if (next_token.type() == TokenType::UNSIGNED_LONG_CONSTANT) {
        return std::make_unique<ConstantExpression>(loc, next_token.literal<unsigned long>());
    } else if (next_token.type() == TokenType::DOUBLE_CONSTANT) {
        return std::make_unique<ConstantExpression>(loc, next_token.literal<double>());
    } else {
        throw ParserError(*this, std::format("Unsupported constant type {}", Token::type_to_string(next_token.type())));
    }
}

const Token& Parser::expect(TokenType expected)
{
    if (!has_tokens()) {
        throw ParserError(*this, std::format("Unexpected end of file. Expected: {}", Token::type_to_string(expected)));
    }

    const Token& actual = m_tokens[i++];
    if (actual.type() != expected) {
        throw ParserError(*this, std::format("Syntax error: Expected '{}' but found '{}' at:\n{}", Token::type_to_string(expected), actual.lexeme(), m_source_manager->get_source_line(actual.source_location())));
    }
    return actual;
}

const Token& Parser::peek(int lh)
{
    size_t j = i + lh - 1;
    if (j >= m_tokens.size()) {
        throw ParserError(*this, std::format("Unexpected end of file. Trying to peek {} look ahead", lh));
    }
    return m_tokens[j];
}

const Token& Parser::last_token()
{
    assert(i > 0 && "last_token fail");
    return m_tokens[i - 1];
}

int Parser::precedence(const Token& token)
{
    switch (token.type()) {
    case TokenType::ASTERISK:
    case TokenType::FORWARD_SLASH:
    case TokenType::PERCENT:
        return 50;
    case TokenType::PLUS:
    case TokenType::MINUS:
        return 45;
    case TokenType::LESS_THAN:
    case TokenType::LESS_THAN_EQUAL:
    case TokenType::GREATER_THAN:
    case TokenType::GREATER_THAN_EQUAL:
        return 35;
    case TokenType::EQUAL:
    case TokenType::NOT_EQUAL:
        return 30;
    case TokenType::LOGICAL_AND:
        return 10;
    case TokenType::LOGICAL_OR:
        return 5;
    case TokenType::QUESTION_MARK:
        return 3;
    case TokenType::ASSIGNMENT:
        return 1;
    default: {
        throw ParserError(*this, "Parser::precedence unexpected token");
    }
    }
}

void Parser::take_token()
{
    i++;
}

bool Parser::has_tokens()
{
    return i < m_tokens.size();
}

bool Parser::is_binary_operator(TokenType type)
{
    switch (type) {
    case TokenType::PLUS:
    case TokenType::MINUS:
    case TokenType::ASTERISK:
    case TokenType::FORWARD_SLASH:
    case TokenType::PERCENT:
    case TokenType::LOGICAL_AND:
    case TokenType::LOGICAL_OR:
    case TokenType::EQUAL:
    case TokenType::NOT_EQUAL:
    case TokenType::LESS_THAN:
    case TokenType::GREATER_THAN:
    case TokenType::LESS_THAN_EQUAL:
    case TokenType::GREATER_THAN_EQUAL:
    case TokenType::ASSIGNMENT:
    case TokenType::QUESTION_MARK: // We treat QUESTION_MARK as a binary operator in parse_expression
        // Add other binary operators as needed
        return true;
    default:
        return false;
    }
}

bool Parser::is_unary_operator(TokenType type)
{
    switch (type) {
    case TokenType::MINUS:
    case TokenType::COMPLEMENT:
    case TokenType::DECREMENT:
    case TokenType::EXCLAMATION_POINT:
        // Add other unary operators as needed
        return true;
    default:
        return false;
    }
}

bool Parser::is_specificer(TokenType type)
{
    switch (type) {
    case TokenType::INT_KW:
    case TokenType::LONG_KW:
    case TokenType::SIGNED_KW:
    case TokenType::UNSIGNED_KW:
    case TokenType::DOUBLE_KW:
    case TokenType::STATIC_KW:
    case TokenType::EXTERN_KW:
        return true;
    default:
        return false;
    }
}

bool Parser::is_type_specificer(TokenType type)
{
    switch (type) {
    case TokenType::INT_KW:
    case TokenType::LONG_KW:
    case TokenType::SIGNED_KW:
    case TokenType::UNSIGNED_KW:
    case TokenType::DOUBLE_KW:
        return true;
    default:
        return false;
    }
}

bool Parser::is_constant(TokenType type)
{
    switch (type) {
    case TokenType::CONSTANT:
    case TokenType::UNSIGNED_CONSTANT:
    case TokenType::LONG_CONSTANT:
    case TokenType::UNSIGNED_LONG_CONSTANT:
    case TokenType::DOUBLE_CONSTANT:
        return true;
    default:
        return false;
    }
}

StorageClass Parser::to_storage_class(TokenType tt)
{
    switch (tt) {
    case TokenType::STATIC_KW:
        return StorageClass::STATIC;
    case TokenType::EXTERN_KW:
        return StorageClass::EXTERN;
    default:
        return StorageClass::NONE;
    }
}

std::pair<std::unique_ptr<Type>, StorageClass> Parser::parse_type_and_storage_class()
{
    ENTER_CONTEXT("parse_type_and_storage_class");

    std::vector<TokenType> type_specifiers;
    std::vector<TokenType> storage_classes;
    const Token* next_token = &peek();
    while (next_token->type() != TokenType::IDENTIFIER) {

        TokenType token_type = next_token->type();
        if (is_type_specificer(token_type)) {
            type_specifiers.push_back(token_type);
        } else if (token_type == TokenType::STATIC_KW || token_type == TokenType::EXTERN_KW) {
            storage_classes.push_back(token_type);
        } else {
            assert(false && "Invalid specifier");
        }
        take_token();
        next_token = &peek();
    }

    std::unique_ptr<Type> type = parse_type_specifier_list(type_specifiers);

    if (storage_classes.size() > 1) {
        throw ParserError(*this, std::format("Specified too many storage_classes {} at:\n{}", storage_classes.size(), m_source_manager->get_source_line(last_token().source_location())));
    }

    StorageClass storage_class = StorageClass::NONE;
    if (storage_classes.size() == 1) {
        storage_class = to_storage_class(storage_classes.at(0));
        assert(storage_class != StorageClass::NONE);
    }
    return { std::move(type), storage_class };
}

Parser::ContextGuard::ContextGuard(ContextStack& context_stack, const std::string& context, std::optional<SourceLocation> source_location)
    : m_context_stack(context_stack)
{
    if (source_location.has_value()) {
        m_context_stack.push_back(std::format("{:<35} line: {:<5} column: {:<3}",
            context,
            source_location.value().line_number,
            source_location.value().column_number));
    } else {
        m_context_stack.push_back(std::format("{}",
            context));
    }
}

Parser::ContextGuard::~ContextGuard()
{
    m_context_stack.pop_back();
}

std::string Parser::context_stack_to_string() const
{
    std::string context_string = std::format("\n==================\nContext Stack:\n");
    for (size_t i = 0; i < m_context_stack.size(); ++i) {
        const auto& str = m_context_stack.at(i);
        context_string += std::format("{}\n", str);
    }
    return context_string;
}


std::tuple<std::string, std::unique_ptr<Type>, std::vector<Identifier>> Parser::process_declarator(const Declarator& declarator, const Type& type)
{
    if(auto id_decl = dynamic_cast<const IdentifierDeclarator*>(&declarator)){
        return {id_decl->identfier, type.clone(), std::vector<Identifier>()};
    } else if(auto ptr_decl = dynamic_cast<const PointerDeclarator*>(&declarator)){
        std::unique_ptr<Type> derived_type = std::make_unique<PointerType>(type.clone());
        return process_declarator(*ptr_decl->inner_declarator,*derived_type);
    }else if(auto fun_decl = dynamic_cast<const FunctionDeclarator*>(&declarator)){
        if(auto fun_id_decl = dynamic_cast<const IdentifierDeclarator*>(fun_decl->declarator.get())){
            std::vector<Identifier> param_names;
            std::vector<std::unique_ptr<Type>> param_types;
            for(auto& param : fun_decl->parameters){
                auto [param_name, param_type, _] = process_declarator(*param.parameter_declarator, *param.parameter_type);
                if(is_type<FunctionType>(*param_type)){
                    throw DeclaratorError("Function pointers in parametrs arent supported");
                }
                param_names.push_back(std::move(param_name));
                param_types.emplace_back((std::move(param_type)));
            }
            std::unique_ptr<Type> derived_type = std::make_unique<FunctionType>(type.clone(), std::move(param_types));
            return {fun_id_decl->identfier, std::move(derived_type), param_names};
        } else{
            throw DeclaratorError("Can't apply additional type derivations to a function type");
        }
    }
}

