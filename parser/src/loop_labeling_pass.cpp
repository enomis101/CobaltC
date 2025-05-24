#include "parser/loop_labeling_pass.h"

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
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(BinaryExpression& node)
{
    node.left_expression->accept(*this);

    node.right_expression->accept(*this);
}

void LoopLabelingPass::visit(ConstantExpression& node)
{
    // Empty method
}

void LoopLabelingPass::visit(ReturnStatement& node)
{
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(FunctionDeclaration& node)
{
    if (node.body.has_value()) {
        node.body.value()->accept(*this);
    }
}

void LoopLabelingPass::visit(Program& node)
{
    // for (auto& fun_decl : node.functions) {
    //     fun_decl->accept(*this);
    // }
}

void LoopLabelingPass::visit(VariableExpression& node)
{
}

void LoopLabelingPass::visit(AssignmentExpression& node)
{
    if (!dynamic_cast<VariableExpression*>(node.left_expression.get())) {
        throw LoopLabelingPassError("In AssignmentExpression: Invalid lvalue!");
    }
    node.left_expression->accept(*this);

    node.right_expression->accept(*this);
}

void LoopLabelingPass::visit(ConditionalExpression& node)
{
    node.condition->accept(*this);

    node.true_expression->accept(*this);

    node.false_expression->accept(*this);
}

void LoopLabelingPass::visit(ExpressionStatement& node)
{
    node.expression->accept(*this);
}

void LoopLabelingPass::visit(IfStatement& node)
{

    node.condition->accept(*this);

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
        item->accept(*this);
    }
}

void LoopLabelingPass::visit(CompoundStatement& node)
{
    node.block->accept(*this);
}

void LoopLabelingPass::visit(BreakStatement& node)
{
    if (m_label_stack.empty()) {
        throw LoopLabelingPassError("In BreakStatement: found break statement with no parent loop");
    }
    node.label.name = m_label_stack.top();
}

void LoopLabelingPass::visit(ContinueStatement& node)
{
    if (m_label_stack.empty()) {
        throw LoopLabelingPassError("In ContinueStatement: found continue statement with no parent loop");
    }
    node.label.name = m_label_stack.top();
}

void LoopLabelingPass::visit(WhileStatement& node)
{
    std::string label = m_name_generator.make_label("while");
    node.label.name = label;
    m_label_stack.push(label);

    node.condition->accept(*this);

    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(DoWhileStatement& node)
{
    std::string label = m_name_generator.make_label("do_while");
    node.label.name = label;
    m_label_stack.push(label);

    node.condition->accept(*this);

    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(ForStatement& node)
{
    std::string label = m_name_generator.make_label("for");
    node.label.name = label;
    m_label_stack.push(label);

    node.init->accept(*this);

    if (node.condition.has_value()) {
        node.condition.value()->accept(*this);
    }

    if (node.post.has_value()) {
        node.post.value()->accept(*this);
    }

    node.body->accept(*this);

    m_label_stack.pop();
}

void LoopLabelingPass::visit(ForInitDeclaration& node)
{
    node.declaration->accept(*this);
}

void LoopLabelingPass::visit(ForInitExpression& node)
{
    if (node.expression.has_value()) {
        node.expression.value()->accept(*this);
    }
}
