#include "parser/parser_printer.h"
#include "parser/parser_ast.h"
#include <fstream>
#include <string>

using namespace parser;

PrinterVisitor::PrinterVisitor()
    : m_node_count(0)
{
}

void PrinterVisitor::generate_dot_file(const std::string& filename, ParserAST& ast)
{
    // Reset state for new file generation
    m_node_count = 0;
    m_node_ids.clear();
    m_dot_content.str("");

    // Start DOT file with digraph definition
    m_dot_content << "digraph ParserAST {\n";
    m_dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

    // Visit the ParserAST to build the DOT representation
    ast.accept(*this);

    // Close the digraph
    m_dot_content << "}\n";

    // Write to file
    std::ofstream out_file(filename);
    if (out_file.is_open()) {
        out_file << m_dot_content.str();
        out_file.close();
    }
}

void PrinterVisitor::visit(Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: " << node.name << "\"];\n";
}

void PrinterVisitor::visit(UnaryExpression& node)
{
    int id = get_node_id(&node);

    std::string label = "UnaryExpression";

    label += "\noperator: " + operator_to_string(node.unary_operator) + "\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(BinaryExpression& node)
{
    int id = get_node_id(&node);

    std::string label = "BinaryExpression";

    label += "\noperator: " + operator_to_string(node.binary_operator) + "\n";

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.left_expression) {
        node.left_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.left_expression.get())
                      << " [label=\"left_expression\"];\n";
    }

    if (node.right_expression) {
        node.right_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.right_expression.get())
                      << " [label=\"right_expression\"];\n";
    }
}

void PrinterVisitor::visit(ConstantExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ConstantExpression\\nvalue: " << node.value << "\"];\n";
}

void PrinterVisitor::visit(ReturnStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ReturnStatement\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(VariableExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"VariableExpression\"];\n";

    // Visit the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(AssignmentExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"AssignmentExpression\"];\n";

    if (node.left_expression) {
        node.left_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.left_expression.get())
                      << " [label=\"left_expression\"];\n";
    }

    if (node.right_expression) {
        node.right_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.right_expression.get())
                      << " [label=\"right_expression\"];\n";
    }
}

void PrinterVisitor::visit(ConditionalExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ConditionalExpression\"];\n";

    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    if (node.true_expression) {
        node.true_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.true_expression.get())
                      << " [label=\"true_expression\"];\n";
    }

    if (node.false_expression) {
        node.false_expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.false_expression.get())
                      << " [label=\"false_expression\"];\n";
    }
}

void PrinterVisitor::visit(FunctionCallExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"FunctionCallExpression\"];\n";

    // Visit the function name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    // Visit each argument
    for (size_t i = 0; i < node.arguments.size(); ++i) {
        if (node.arguments[i]) {
            node.arguments[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.arguments[i].get())
                          << " [label=\"arguments[" << i << "]\"];\n";
        }
    }
}

void PrinterVisitor::visit(ExpressionStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ExpressionStatement\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(IfStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"IfStatement\"];\n";

    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    if (node.then_statement) {
        node.then_statement->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.then_statement.get())
                      << " [label=\"then_statement\"];\n";
    }

    if (node.else_statement.has_value() && node.else_statement.value()) {
        node.else_statement.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.else_statement.value().get())
                      << " [label=\"else_statement\"];\n";
    }
}

void PrinterVisitor::visit(NullStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"NullStatement\"];\n";
}

// In parser/src/parser_printer.cpp
// Add this helper function to convert StorageClass to string:

std::string PrinterVisitor::storage_class_to_string(StorageClass sc)
{
    switch (sc) {
    case StorageClass::NONE:
        return "NONE";
    case StorageClass::STATIC:
        return "STATIC";
    case StorageClass::EXTERN:
        return "EXTERN";
    default:
        return "UNKNOWN";
    }
}

std::string PrinterVisitor::declaration_scope_to_string(DeclarationScope scope)
{
    switch (scope) {
    case DeclarationScope::File:
        return "File";
    case DeclarationScope::Block:
        return "Block";
    default:
        return "UNKNOWN";
    }
}

// Update the VariableDeclaration visit method to include storage class:
void PrinterVisitor::visit(VariableDeclaration& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"VariableDeclaration\\nstorage_class: "
                  << storage_class_to_string(node.storage_class) << "\"];\n"
                  << "\\ndeclaration_scope: " << declaration_scope_to_string(node.scope) << "\"];\n";

    // Visit the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";

    // Visit the initializer expression if present
    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.value().get())
                      << " [label=\"initializer\"];\n";
    }
}

// Update the FunctionDeclaration visit method to include storage class:
void PrinterVisitor::visit(FunctionDeclaration& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"FunctionDeclaration\\nname: " << node.name.name
                  << "\\nstorage_class: " << storage_class_to_string(node.storage_class) << "\"];\n"
                  << "\\ndeclaration_scope: " << declaration_scope_to_string(node.scope) << "\"];\n";

    // Visit the name identifier
    node.name.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.name)
                  << " [label=\"name\"];\n";

    // Visit each parameter
    for (size_t i = 0; i < node.params.size(); ++i) {
        node.params[i].accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.params[i])
                      << " [label=\"params[" << i << "]\"];\n";
    }

    // Visit the body if present
    if (node.body.has_value() && node.body.value()) {
        node.body.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.value().get())
                      << " [label=\"body\"];\n";
    }
}

std::string PrinterVisitor::operator_to_string(UnaryOperator op)
{
    switch (op) {
    case UnaryOperator::COMPLEMENT:
        return "Complement";
    case UnaryOperator::NEGATE:
        return "Negate";
    case UnaryOperator::NOT:
        return "Not";
    default:
        return "unknown";
    }
}

std::string PrinterVisitor::operator_to_string(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "Add";
    case BinaryOperator::SUBTRACT:
        return "Subtract";
    case BinaryOperator::MULTIPLY:
        return "Multiply";
    case BinaryOperator::DIVIDE:
        return "Divide";
    case BinaryOperator::REMAINDER:
        return "Remainder";
    case BinaryOperator::AND:
        return "And";
    case BinaryOperator::OR:
        return "Or";
    case BinaryOperator::EQUAL:
        return "Equal";
    case BinaryOperator::NOT_EQUAL:
        return "NotEqual";
    case BinaryOperator::LESS_THAN:
        return "LessThan";
    case BinaryOperator::LESS_OR_EQUAL:
        return "LessOrEqual";
    case BinaryOperator::GREATER_THAN:
        return "GreaterThan";
    case BinaryOperator::GREATER_OR_EQUAL:
        return "GreaterOrEqual";
    default:
        return "unknown";
    }
}

void PrinterVisitor::visit(Block& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Block\"];\n";

    for (size_t i = 0; i < node.items.size(); ++i) {
        if (node.items[i]) {
            node.items[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.items[i].get())
                          << " [label=\"items[" << i << "]\"];\n";
        }
    }
}

void PrinterVisitor::visit(CompoundStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"CompoundStatement\"];\n";

    // Visit each block item in the body
    if (node.block) {
        node.block->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.block.get())
                      << " [label=\"block\"];\n";
    }
}

void PrinterVisitor::visit(Program& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Program\", color=blue, style=filled, fillcolor=lightblue];\n";

    // Process each function in the vector
    for (size_t i = 0; i < node.declarations.size(); ++i) {
        if (node.declarations[i]) {
            node.declarations[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.declarations[i].get())
                          << " [label=\"declarations[" << i << "]\"];\n";
        }
    }
}

int PrinterVisitor::get_node_id(const ParserAST* node)
{
    if (m_node_ids.find(node) == m_node_ids.end()) {
        m_node_ids[node] = m_node_count++;
    }
    return m_node_ids[node];
}

void PrinterVisitor::visit(BreakStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"BreakStatement\"];\n";

    // Visit the label identifier
    node.label.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                  << " [label=\"label\"];\n";
}

void PrinterVisitor::visit(ContinueStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ContinueStatement\"];\n";

    // Visit the label identifier
    node.label.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                  << " [label=\"label\"];\n";
}

void PrinterVisitor::visit(WhileStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"WhileStatement\"];\n";

    // Visit the condition
    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    // Visit the body
    if (node.body) {
        node.body->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                      << " [label=\"body\"];\n";
    }

    // Visit the label
    node.label.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                  << " [label=\"label\"];\n";
}

void PrinterVisitor::visit(DoWhileStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"DoWhileStatement\"];\n";

    // Visit the condition
    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    // Visit the body
    if (node.body) {
        node.body->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                      << " [label=\"body\"];\n";
    }

    // Visit the label
    node.label.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                  << " [label=\"label\"];\n";
}

void PrinterVisitor::visit(ForStatement& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ForStatement\"];\n";

    // Visit the initialization
    if (node.init) {
        node.init->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.init.get())
                      << " [label=\"init\"];\n";
    }

    // Visit the condition if present
    if (node.condition.has_value() && node.condition.value()) {
        node.condition.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.value().get())
                      << " [label=\"condition\"];\n";
    }

    // Visit the post-expression if present
    if (node.post.has_value() && node.post.value()) {
        node.post.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.post.value().get())
                      << " [label=\"post\"];\n";
    }

    // Visit the body
    if (node.body) {
        node.body->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                      << " [label=\"body\"];\n";
    }

    // Visit the label
    node.label.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                  << " [label=\"label\"];\n";
}

void PrinterVisitor::visit(ForInitDeclaration& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ForInitDeclaration\"];\n";

    // Visit the declaration
    if (node.declaration) {
        node.declaration->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.declaration.get())
                      << " [label=\"declaration\"];\n";
    }
}

void PrinterVisitor::visit(ForInitExpression& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ForInitExpression\"];\n";

    // Visit the expression if present
    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.value().get())
                      << " [label=\"expression\"];\n";
    }
}
