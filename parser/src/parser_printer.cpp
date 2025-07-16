#include "parser/parser_printer.h"
#include "common/error/internal_compiler_error.h"
#include "parser/parser_ast.h"
#include <fstream>
#include <sstream>
#include <string>

using namespace parser;

PrinterVisitor::PrinterVisitor(bool require_valid_type)
    : m_node_count(0)
    , m_require_valid_type(require_valid_type)
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

std::string PrinterVisitor::escape_string(const std::string& str)
{
    std::string result;
    for (char c : str) {
        switch (c) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
            break;
        }
    }
    return result;
}

std::string PrinterVisitor::constant_value_to_string(const ConstantType& value)
{
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::monostate>) {
            return "[uninitialized]";
        } else if constexpr (std::is_same_v<T, int>) {
            return std::to_string(v);
        } else if constexpr (std::is_same_v<T, long>) {
            return std::to_string(v) + "L";
        } else if constexpr (std::is_same_v<T, unsigned int>) {
            return std::to_string(v) + "U";
        } else if constexpr (std::is_same_v<T, unsigned long>) {
            return std::to_string(v) + "UL";
        } else if constexpr (std::is_same_v<T, double>) {
            return std::to_string(v);
        } else {
            return "[unknown_type]";
        }
    },
        value);
}

std::string PrinterVisitor::type_to_string(const std::unique_ptr<Type>& type)
{
    if (type) {
        return "\\ntype: " + type->to_string();
    } else if (m_require_valid_type) {
        throw InternalCompilerError("Type must be valid");
    } else {
        return "";
    }
}

void PrinterVisitor::visit(Identifier& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Identifier\\nname: "
                  << escape_string(node.name) << "\"];\n";
}

void PrinterVisitor::visit(UnaryExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "UnaryExpression\\noperator: " + operator_to_string(node.unary_operator);

    // Add type information if available
    label += type_to_string(node.type);

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
    std::string label = "BinaryExpression\\noperator: " + operator_to_string(node.binary_operator);

    // Add type information if available
    label += type_to_string(node.type);

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
    std::string label = "ConstantExpression\\nvalue: " + constant_value_to_string(node.value);

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";
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
    std::string label = "VariableExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Visit the identifier
    node.identifier.accept(*this);
    m_dot_content << "  node" << id << " -> node" << get_node_id(&node.identifier)
                  << " [label=\"identifier\"];\n";
}

void PrinterVisitor::visit(CastExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "CastExpression";

    // Add type information if available
    if (node.type) {
        label += "\\nresult_type: " + type_to_string(node.type);
    } else {
        label += "\\nresult_type: [no_type]";
    }

    if (node.target_type) {
        label += "\\ntarget_type: " + type_to_string(node.target_type);
    } else {
        label += "\\ntarget_type: [no_type]";
    }

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Visit the expression being cast
    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(AssignmentExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "AssignmentExpression";

    // Add type information if available
    label += type_to_string(node.type);

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

void PrinterVisitor::visit(ConditionalExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "ConditionalExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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
    std::string label = "FunctionCallExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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

void PrinterVisitor::visit(DereferenceExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "DereferenceExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(AddressOfExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "AddressOfExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(SubscriptExpression& node)
{
    int id = get_node_id(&node);
    std::string label = "SubscriptExpression";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression1) {
        node.expression1->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression1.get())
                      << " [label=\"expression1\"];\n";
    }

    if (node.expression2) {
        node.expression2->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression2.get())
                      << " [label=\"expression2\"];\n";
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

void PrinterVisitor::visit(SingleInitializer& node)
{
    int id = get_node_id(&node);
    std::string label = "SingleInitializer";

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    if (node.expression) {
        node.expression->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                      << " [label=\"expression\"];\n";
    }
}

void PrinterVisitor::visit(CompoundInitializer& node)
{
    int id = get_node_id(&node);
    std::string label = "CompoundInitializer\\ninitializers: " + std::to_string(node.initializer_list.size());

    // Add type information if available
    label += type_to_string(node.type);

    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Visit each initializer in the list
    for (size_t i = 0; i < node.initializer_list.size(); ++i) {
        if (node.initializer_list[i]) {
            node.initializer_list[i]->accept(*this);
            m_dot_content << "  node" << id << " -> node" << get_node_id(node.initializer_list[i].get())
                          << " [label=\"initializer[" << i << "]\"];\n";
        }
    }
}

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

void PrinterVisitor::visit(VariableDeclaration& node)
{
    int id = get_node_id(&node);
    std::string label = "VariableDeclaration\\nstorage_class: " + storage_class_to_string(node.storage_class) + "\\ndeclaration_scope: " + declaration_scope_to_string(node.scope);

    // Add type information if available
    label += type_to_string(node.type);
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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

void PrinterVisitor::visit(FunctionDeclaration& node)
{
    int id = get_node_id(&node);
    std::string label = "FunctionDeclaration\\nname: " + escape_string(node.name.name) + "\\nstorage_class: " + storage_class_to_string(node.storage_class) + "\\ndeclaration_scope: " + declaration_scope_to_string(node.scope);

    // Add type information if available
    label += type_to_string(node.type);
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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
        return "~";
    case UnaryOperator::NEGATE:
        return "-";
    case UnaryOperator::NOT:
        return "!";
    default:
        return "unknown";
    }
}

std::string PrinterVisitor::operator_to_string(BinaryOperator op)
{
    switch (op) {
    case BinaryOperator::ADD:
        return "+";
    case BinaryOperator::SUBTRACT:
        return "-";
    case BinaryOperator::MULTIPLY:
        return "*";
    case BinaryOperator::DIVIDE:
        return "/";
    case BinaryOperator::REMAINDER:
        return "%";
    case BinaryOperator::AND:
        return "&&";
    case BinaryOperator::OR:
        return "||";
    case BinaryOperator::EQUAL:
        return "==";
    case BinaryOperator::NOT_EQUAL:
        return "!=";
    case BinaryOperator::LESS_THAN:
        return "<";
    case BinaryOperator::LESS_OR_EQUAL:
        return "<=";
    case BinaryOperator::GREATER_THAN:
        return ">";
    case BinaryOperator::GREATER_OR_EQUAL:
        return ">=";
    default:
        return "unknown";
    }
}

void PrinterVisitor::visit(Block& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Block\\nitems: " << node.items.size() << "\"];\n";

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

    if (node.block) {
        node.block->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.block.get())
                      << " [label=\"block\"];\n";
    }
}

void PrinterVisitor::visit(Program& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"Program\\ndeclarations: " << node.declarations.size()
                  << "\", color=blue, style=filled, fillcolor=lightblue];\n";

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
    std::string label = "BreakStatement";
    if (!node.label.name.empty()) {
        label += "\\nlabel: " + escape_string(node.label.name);
    }
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Only visit label if it has a name (not empty)
    if (!node.label.name.empty()) {
        node.label.accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                      << " [label=\"label\"];\n";
    }
}

void PrinterVisitor::visit(ContinueStatement& node)
{
    int id = get_node_id(&node);
    std::string label = "ContinueStatement";
    if (!node.label.name.empty()) {
        label += "\\nlabel: " + escape_string(node.label.name);
    }
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Only visit label if it has a name (not empty)
    if (!node.label.name.empty()) {
        node.label.accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                      << " [label=\"label\"];\n";
    }
}

void PrinterVisitor::visit(WhileStatement& node)
{
    int id = get_node_id(&node);
    std::string label = "WhileStatement";
    if (!node.label.name.empty()) {
        label += "\\nlabel: " + escape_string(node.label.name);
    }
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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

    // Only visit label if it has a name (not empty)
    if (!node.label.name.empty()) {
        node.label.accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                      << " [label=\"label\"];\n";
    }
}

void PrinterVisitor::visit(DoWhileStatement& node)
{
    int id = get_node_id(&node);
    std::string label = "DoWhileStatement";
    if (!node.label.name.empty()) {
        label += "\\nlabel: " + escape_string(node.label.name);
    }
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

    // Visit the body first (do-while executes body first)
    if (node.body) {
        node.body->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                      << " [label=\"body\"];\n";
    }

    // Visit the condition
    if (node.condition) {
        node.condition->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.condition.get())
                      << " [label=\"condition\"];\n";
    }

    // Only visit label if it has a name (not empty)
    if (!node.label.name.empty()) {
        node.label.accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                      << " [label=\"label\"];\n";
    }
}

void PrinterVisitor::visit(ForStatement& node)
{
    int id = get_node_id(&node);
    std::string label = "ForStatement";
    if (!node.label.name.empty()) {
        label += "\\nlabel: " + escape_string(node.label.name);
    }
    m_dot_content << "  node" << id << " [label=\"" << label << "\"];\n";

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

    // Only visit label if it has a name (not empty)
    if (!node.label.name.empty()) {
        node.label.accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(&node.label)
                      << " [label=\"label\"];\n";
    }
}

void PrinterVisitor::visit(ForInitDeclaration& node)
{
    int id = get_node_id(&node);
    m_dot_content << "  node" << id << " [label=\"ForInitDeclaration\"];\n";

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

    if (node.expression.has_value() && node.expression.value()) {
        node.expression.value()->accept(*this);
        m_dot_content << "  node" << id << " -> node" << get_node_id(node.expression.value().get())
                      << " [label=\"expression\"];\n";
    }
}
