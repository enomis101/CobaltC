#include "parser/parser.h"
#include <format>
#include <sstream>
#include <vector>

using namespace parser;

std::shared_ptr<Program> Parser::parse_program()
{
    std::vector<std::unique_ptr<FunctionDeclaration>> funcs;
    while (has_tokens()) {
        funcs.emplace_back(parse_function_declaration());
    }
    return std::make_shared<Program>(std::move(funcs));
}

std::unique_ptr<Block> Parser::parse_block()
{
    expect(TokenType::OPEN_BRACE);
    const Token* next_token = has_tokens() ? &peek() : nullptr;
    std::vector<std::unique_ptr<BlockItem>> body;
    while (next_token && next_token->type() != TokenType::CLOSE_BRACE) {
        body.emplace_back(parse_block_item());
        next_token = has_tokens() ? &peek() : nullptr;
    }
    expect(TokenType::CLOSE_BRACE);
    return std::make_unique<Block>(std::move(body));
}

std::unique_ptr<BlockItem> Parser::parse_block_item()
{
    const Token& next_token = peek();
    if (next_token.type() == TokenType::INT_KW) {
        return parse_declaration();
    } else {
        return parse_statement();
    }
}

std::vector<Identifier> Parser::parse_parameter_list()
{
    const Token* next_token = &peek();
    std::vector<Identifier> res;
    if (next_token->type() == TokenType::VOID_KW) {
        expect(TokenType::VOID_KW);
        return res;
    }

    while (true) {
        expect(TokenType::INT_KW);
        const Token& identifier_token = expect(TokenType::IDENTIFIER);
        res.push_back(identifier_token.lexeme());
        next_token = &peek();
        if (next_token->type() == TokenType::CLOSE_PAREN) {
            break;
        }
        expect(TokenType::COMMA);
    }

    return res;
}

std::unique_ptr<Declaration> Parser::parse_declaration()
{
    const Token& next_token = peek(3); // Look ahead 3 tokens
    if (next_token.type() == TokenType::OPEN_PAREN) {
        return parse_function_declaration();
    }
    return parse_variable_declaration();
}

std::unique_ptr<FunctionDeclaration> Parser::parse_function_declaration()
{
    expect(TokenType::INT_KW);
    const Token& identifier_token = expect(TokenType::IDENTIFIER);

    expect(TokenType::OPEN_PAREN);
    std::vector<Identifier> param_list = parse_parameter_list();
    expect(TokenType::CLOSE_PAREN);

    const Token& next_token = peek();
    std::unique_ptr<Block> body = nullptr;
    if (next_token.type() == TokenType::SEMICOLON) {
        expect(TokenType::SEMICOLON);
    } else {
        body = parse_block();
    }

    return std::make_unique<FunctionDeclaration>(identifier_token.lexeme(), param_list, std::move(body));
}

std::unique_ptr<VariableDeclaration> Parser::parse_variable_declaration()
{
    expect(TokenType::INT_KW);
    const Token& identifier_token = expect(TokenType::IDENTIFIER);
    const Token& next_token = peek();

    if (next_token.type() == TokenType::SEMICOLON) {
        expect(TokenType::SEMICOLON);
        return std::make_unique<VariableDeclaration>(identifier_token.lexeme());
    }

    expect(TokenType::ASSIGNMENT);
    std::unique_ptr<Expression> init_expr = parse_expression();
    expect(TokenType::SEMICOLON);
    return std::make_unique<VariableDeclaration>(identifier_token.lexeme(), std::move(init_expr));
}

std::unique_ptr<ForInit> Parser::parse_for_init()
{
    const Token& next_token = peek();
    if (next_token.type() == TokenType::INT_KW) {
        std::unique_ptr<VariableDeclaration> decl = parse_variable_declaration();
        return std::make_unique<ForInitDeclaration>(std::move(decl));
    } else {
        std::unique_ptr<Expression> e = (next_token.type() == TokenType::SEMICOLON) ? nullptr : parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ForInitExpression>(std::move(e));
    }
}

std::unique_ptr<Statement> Parser::parse_statement()
{
    const Token& next_token = peek();
    switch (next_token.type()) {
    case TokenType::RETURN_KW: {
        expect(TokenType::RETURN_KW);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ReturnStatement>(std::move(expr));
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
        return std::make_unique<IfStatement>(std::move(expr), std::move(then_statement), std::move(else_statement));
    }
    case TokenType::OPEN_BRACE: {
        std::unique_ptr<Block> block = parse_block();
        return std::make_unique<CompoundStatement>(std::move(block));
    }
    case TokenType::SEMICOLON: {
        expect(TokenType::SEMICOLON);
        return std::make_unique<NullStatement>();
    }
    case TokenType::BREAK_KW: {
        expect(TokenType::BREAK_KW);
        expect(TokenType::SEMICOLON);
        return std::make_unique<BreakStatement>();
    }
    case TokenType::CONTINUE_KW: {
        expect(TokenType::CONTINUE_KW);
        expect(TokenType::SEMICOLON);
        return std::make_unique<ContinueStatement>();
    }
    case TokenType::WHILE_KW: {
        expect(TokenType::WHILE_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        std::unique_ptr<Statement> statement = parse_statement();
        return std::make_unique<WhileStatement>(std::move(expr), std::move(statement));
    }
    case TokenType::DO_KW: {
        expect(TokenType::DO_KW);
        std::unique_ptr<Statement> statement = parse_statement();
        expect(TokenType::WHILE_KW);
        expect(TokenType::OPEN_PAREN);
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        expect(TokenType::SEMICOLON);
        return std::make_unique<DoWhileStatement>(std::move(expr), std::move(statement));
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
        return std::make_unique<ForStatement>(std::move(for_init), std::move(cond), std::move(post), std::move(statement));
    }
    default: {
        std::unique_ptr<Expression> expr = parse_expression();
        expect(TokenType::SEMICOLON);
        return std::make_unique<ExpressionStatement>(std::move(expr));
    }
    }
}

std::unique_ptr<Expression> Parser::parse_conditional_middle()
{
    expect(TokenType::QUESTION_MARK);
    std::unique_ptr<Expression> expr = parse_expression(0); // reset back to zero precedence level
    expect(TokenType::COLON);
    return expr;
}

// Implement precedence climbing
std::unique_ptr<Expression> Parser::parse_expression(int min_prec)
{
    std::unique_ptr<Expression> left;

    left = parse_factor();
    if (!has_tokens()) {
        return left;
    }

    const Token* next_token = &peek();
    while (next_token && is_binary_operator(next_token->type()) && precedence(*next_token) >= min_prec) {
        if (next_token->type() == TokenType::ASSIGNMENT) { //= must be rigth associative a = b = c --> a  = (b = c)
            take_token();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token)); // different than binary operator because it's rigth associative
            left = std::make_unique<AssignmentExpression>(std::move(left), std::move(right));
        } else if (next_token->type() == TokenType::QUESTION_MARK) {
            // QUESTION_MARK consumed by parse_conditional_middle
            std::unique_ptr<Expression> middle = parse_conditional_middle();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token)); // different than binary operator because it's rigth associative
            left = std::make_unique<ConditionalExpression>(std::move(left), std::move(middle), std::move(right));
        } else {
            BinaryOperator op = parse_binary_operator();
            std::unique_ptr<Expression> right = parse_expression(precedence(*next_token) + 1);
            left = std::make_unique<BinaryExpression>(op, std::move(left), std::move(right));
        }

        next_token = has_tokens() ? &peek() : nullptr;
    }
    return left;
}

std::unique_ptr<Expression> Parser::parse_factor()
{
    const Token& next_token = peek();
    if (next_token.type() == TokenType::CONSTANT) {
        take_token();
        return std::make_unique<ConstantExpression>(next_token.literal<int>());
    } else if (is_unary_operator(next_token.type())) {
        UnaryOperator op = parse_unary_operator();
        std::unique_ptr<Expression> expr = parse_factor();
        return std::make_unique<UnaryExpression>(op, std::move(expr));
    } else if (next_token.type() == TokenType::OPEN_PAREN) {
        take_token();
        std::unique_ptr<Expression> res = parse_expression();
        expect(TokenType::CLOSE_PAREN);
        return res;
    } else if (next_token.type() == TokenType::IDENTIFIER) {
        const Token& identifier_token = expect(TokenType::IDENTIFIER);
        const Token* new_next_token = &peek();
        if (new_next_token->type() != TokenType::OPEN_PAREN) {
            return std::make_unique<VariableExpression>(identifier_token.lexeme());
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
        return std::make_unique<FunctionCallExpression>(identifier_token.lexeme(), std::move(args));
    } else {
        throw ParserError(std::format("Malformed Factor {}", next_token.to_string()));
    }
}

UnaryOperator Parser::parse_unary_operator()
{
    static const std::unordered_map<TokenType, UnaryOperator> unary_op_map = {
        { TokenType::MINUS, UnaryOperator::NEGATE },
        { TokenType::COMPLEMENT, UnaryOperator::COMPLEMENT },
        { TokenType::EXCLAMATION_POINT, UnaryOperator::NOT }
    };

    const Token& next_token = peek();
    auto it = unary_op_map.find(next_token.type());
    if (it != unary_op_map.end()) {
        take_token();
        return it->second;
    }
    throw ParserError(std::format("In UnaryOperator: got {}", Token::type_to_string(next_token.type())));
}

BinaryOperator Parser::parse_binary_operator()
{
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
    if (it != binary_op_map.end()) {
        take_token();
        return it->second;
    }
    throw ParserError(std::format("In BinaryOperator: got {}", Token::type_to_string(next_token.type())));
}

const Token& Parser::expect(TokenType expected)
{
    if (!has_tokens()) {
        throw ParserError(std::format(
            "Unexpected end of file. Expected: {}",
            Token::type_to_string(expected)));
    }

    const Token& actual = m_tokens[i++];
    if (actual.type() != expected) {
        throw ParserError(std::format(
            "Syntax error at line {}: Expected '{}' but found '{}'",
            actual.line(),
            Token::type_to_string(expected),
            actual.lexeme()));
    }
    return actual;
}

const Token& Parser::peek(int lh)
{
    size_t j = i + lh - 1;
    if (j >= m_tokens.size()) {
        throw ParserError(std::format("Unexpected end of file. Trying to peek {} look ahead", lh));
    }
    return m_tokens[j];
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
        throw ParserError("Parser::precedence unexpected token");
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
