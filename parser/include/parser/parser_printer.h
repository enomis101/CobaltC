#pragma once
#include "parser/parser_ast.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace parser {

class PrinterVisitor : public Visitor {
public:
    PrinterVisitor()
        : node_count(0)
    {
    }

    // Generate DOT file from the AST
    void generate_dot_file(const std::string& filename, AST& ast)
    {
        // Reset state for new file generation
        node_count = 0;
        node_ids.clear();
        dot_content.str("");

        // Start DOT file with digraph definition
        dot_content << "digraph AST {\n";
        dot_content << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";

        // Visit the AST to build the DOT representation
        ast.accept(*this);

        // Close the digraph
        dot_content << "}\n";

        // Write to file
        std::ofstream out_file(filename);
        if (out_file.is_open()) {
            out_file << dot_content.str();
            out_file.close();
        }
    }

    // Implementation of visitor interface methods
    void visit(Identifier& node) override
    {
        int id = get_node_id(&node);
        dot_content << "  node" << id << " [label=\"Identifier\\nname: " << node.name << "\"];\n";
    }

    void visit(ConstantExpression& node) override
    {
        int id = get_node_id(&node);
        dot_content << "  node" << id << " [label=\"ConstantExpression\\nvalue: " << node.value << "\"];\n";
    }

    void visit(ReturnStatement& node) override
    {
        int id = get_node_id(&node);
        dot_content << "  node" << id << " [label=\"ReturnStatement\"];\n";

        if (node.expression) {
            node.expression->accept(*this);
            dot_content << "  node" << id << " -> node" << get_node_id(node.expression.get())
                        << " [label=\"expression\"];\n";
        }
    }

    void visit(Function& node) override
    {
        int id = get_node_id(&node);
        dot_content << "  node" << id << " [label=\"Function\"];\n";

        if (node.name) {
            node.name->accept(*this);
            dot_content << "  node" << id << " -> node" << get_node_id(node.name.get())
                        << " [label=\"name\"];\n";
        }

        if (node.body) {
            node.body->accept(*this);
            dot_content << "  node" << id << " -> node" << get_node_id(node.body.get())
                        << " [label=\"body\"];\n";
        }
    }

    void visit(Program& node) override
    {
        int id = get_node_id(&node);
        dot_content << "  node" << id << " [label=\"Program\", color=blue, style=filled, fillcolor=lightblue];\n";

        if (node.function) {
            node.function->accept(*this);
            dot_content << "  node" << id << " -> node" << get_node_id(node.function.get())
                        << " [label=\"function\"];\n";
        }
    }

private:
    // Get or assign a unique ID for each node
    int get_node_id(const AST* node)
    {
        if (node_ids.find(node) == node_ids.end()) {
            node_ids[node] = node_count++;
        }
        return node_ids[node];
    }

    int node_count;                               // Counter for generating unique node IDs
    std::unordered_map<const AST*, int> node_ids; // Maps AST nodes to their unique IDs
    std::stringstream dot_content;                // Buffer for dot file content
};

} // namespace parser
