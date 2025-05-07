#include "parser/loop_labeling_pass.h"
#include <format>

using namespace parser;

void LoopLabelingPass::run()
{
    m_ast->accept(*this);
}

void LoopLabelingPass::visit(Identifier& node)
{
    // Empty method
}

void LoopLabelingPass::visit(UnaryExpression& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In UnaryExpression: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(BinaryExpression& node)
{
    if (!node.left_expression) {
        throw SemanticAnalyzerError("In BinaryExpression: Left expression pointer is null");
    }
    node.left_expression->accept(*this);

    if (!node.right_expression) {
        throw SemanticAnalyzerError("In BinaryExpression: Right expression pointer is null");
    }
    node.right_expression->accept(*this);
}

void LoopLabelingPass::visit(ConstantExpression& node)
{
    // Empty method
}

void LoopLabelingPass::visit(ReturnStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ReturnStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(Function& node)
{
    if (!node.name) {
        throw SemanticAnalyzerError("In Function: Name pointer is null");
    }
    node.name->accept(*this);

    if (!node.body) {
        throw SemanticAnalyzerError("In Function: Body pointer is null");
    }
    node.body->accept(*this);
}

void LoopLabelingPass::visit(Program& node)
{
    if (!node.function) {
        throw SemanticAnalyzerError("In Program: Function pointer is null");
    }
    node.function->accept(*this);
}

void LoopLabelingPass::visit(VariableExpression& node)
{
}

void LoopLabelingPass::visit(AssignmentExpression& node)
{
    if (!node.left_expression) {
        throw SemanticAnalyzerError("In AssignmentExpression: Left expression pointer is null");
    }

    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw SemanticAnalyzerError("In AssignmentExpression: Invalid lvalue!");
    }
    node.left_expression->accept(*this);

    if (!node.right_expression) {
        throw SemanticAnalyzerError("In AssignmentExpression: Right expression pointer is null");
    }
    node.right_expression->accept(*this);
}

void LoopLabelingPass::visit(ConditionalExpression& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In ConditionalExpression: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.true_expression) {
        throw SemanticAnalyzerError("In ConditionalExpression: True expression pointer is null");
    }
    node.true_expression->accept(*this);

    if (!node.false_expression) {
        throw SemanticAnalyzerError("In ConditionalExpression: False expression pointer is null");
    }
    node.false_expression->accept(*this);
}

void LoopLabelingPass::visit(ExpressionStatement& node)
{
    if (!node.expression) {
        throw SemanticAnalyzerError("In ExpressionStatement: Expression pointer is null");
    }
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(IfStatement& node)
{
    if (!node.condition) {
        throw SemanticAnalyzerError("In IfStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.then_statement) {
        throw SemanticAnalyzerError("In IfStatement: Then statement pointer is null");
    }
    node.then_statement->accept(*this);

    if (node.else_statement.has_value()) {
        node.else_statement.value()->accept(*this);
    }
}

void LoopLabelingPass::visit(NullStatement& node)
{
    // Empty method
}

void LoopLabelingPass::visit(VariableDeclaration& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}

void LoopLabelingPass::visit(Block& node)
{
    for (auto& item : node.items) {
        if (!item) {
            throw SemanticAnalyzerError("In Block: item pointer is null");
        }
        item->accept(*this);
    }
}

void LoopLabelingPass::visit(CompoundStatement& node)
{
    if (!node.block) {
        throw SemanticAnalyzerError("In CompoundStatement: block pointer is null");
    }
    node.block->accept(*this);
}

void LoopLabelingPass::visit(BreakStatement& node)  
{
    if(m_label_stack.empty()){
        throw SemanticAnalyzerError("In BreakStatement: found break statement with no parent loop");
    }
    node.label.name = m_label_stack.top();
}

void LoopLabelingPass::visit(ContinueStatement& node)  
{
    if(m_label_stack.empty()){
        throw SemanticAnalyzerError("In ContinueStatement: found continue statement with no parent loop");
    }
    node.label.name = m_label_stack.top();
}


void LoopLabelingPass::visit(WhileStatement& node)  
{
    std::string label = m_name_generator.make_label("while");
    node.label.name = label;
    m_label_stack.push(label);

    if (!node.condition) {
        throw SemanticAnalyzerError("In WhileStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.body) {
        throw SemanticAnalyzerError("In WhileStatement: Then body pointer is null");
    }
    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(DoWhileStatement& node)   
{
    std::string label = m_name_generator.make_label("do_while");
    node.label.name = label;
    m_label_stack.push(label);

    if (!node.condition) {
        throw SemanticAnalyzerError("In DoWhileStatement: Condition pointer is null");
    }
    node.condition->accept(*this);

    if (!node.body) {
        throw SemanticAnalyzerError("In DoWhileStatement: Then body pointer is null");
    }
    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(ForStatement& node)  
{
    std::string label = m_name_generator.make_label("for");
    node.label.name = label;
    m_label_stack.push(label);

    if (!node.init) {
        throw SemanticAnalyzerError("In ForStatement: init pointer is null");
    }
    node.init->accept(*this);

    if(node.condition.has_value())
    {
        if (!node.condition.value()) {
            throw SemanticAnalyzerError("In ForStatement: condition pointer is null");
        }
        node.condition.value()->accept(*this);
    }

    if(node.post.has_value())
    {
        if (!node.post.value()) {
            throw SemanticAnalyzerError("In ForStatement: post pointer is null");
        }
        node.post.value()->accept(*this);
    }

    if (!node.body) {
        throw SemanticAnalyzerError("In ForStatement: Then body pointer is null");
    }
    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(ForInitDeclaration& node)  
{
    if (!node.declaration) {
        throw SemanticAnalyzerError("In ForInitDeclaration: declaration pointer is null");
    }
    node.declaration->accept(*this);
}

void LoopLabelingPass::visit(ForInitExpression& node)  
{
    if(node.expression.has_value())
    {
        if (!node.expression.value()) {
            throw SemanticAnalyzerError("In ForInitExpression: expression pointer is null");
        }
        node.expression.value()->accept(*this);
    }
}
